#include "GrpcServerManager.h"

namespace org { namespace bcom { namespace xpcf {

GrpcServerManager::GrpcServerManager():ConfigurableBase(toMap<GrpcServerManager>())
{
    std::cout<<"ConfigurableBase" << std::endl;

    declareInterface<IGrpcServerManager>(this);
    declareProperty("server_address",m_serverAddress);
    declareProperty("server_port",m_serverPort);
    declareProperty("serverCredentials",m_serverCredentials);
    declareProperty("max_receive_message_size", m_receiveMessageMaxSize);
    declareProperty("max_send_message_size", m_sendMessageMaxSize);
    declareInjectable<IGrpcService>(m_services);

    std::cout<< "this->getNbInterfaces() = " << this->getNbInterfaces() << std::endl;
}

GrpcServerManager::~GrpcServerManager()
{
    std::cout<<"~GrpcServerManager" << std::endl;
}

void GrpcServerManager::unloadComponent ()
{
    std::cout<<"unloadComponent" << std::endl;

    // provide component cleanup strategy
    // default strategy is to delete self, uncomment following line in this case :
    delete this;
    return;
}

XPCFErrorCode GrpcServerManager::onConfigured()
{
    if (m_receiveMessageMaxSize > 0) {
        m_builder.SetMaxReceiveMessageSize(m_receiveMessageMaxSize);
        std::cout << "Set max receive message size to: " << m_receiveMessageMaxSize << std::endl;
    }

    if (m_sendMessageMaxSize > 0) {
        m_builder.SetMaxSendMessageSize(m_sendMessageMaxSize);
        std::cout << "Set max send message size to: " << m_sendMessageMaxSize << std::endl;
    }
    return XPCFErrorCode::_SUCCESS;
}

void GrpcServerManager::registerService(grpc::Service * service)
{
    m_builder.RegisterService(service);
}

void GrpcServerManager::registerService(const grpc::string & host, grpc::Service * service)
{
    m_builder.RegisterService(host, service);
}

void GrpcServerManager::registerService(SRef<IGrpcService> service)
{
    registerService(service->getService());
}

void GrpcServerManager::registerService(const grpc::string & host, SRef<IGrpcService> service)
{
    registerService(host, service->getService());
}

void GrpcServerManager::runServer()
{
    m_builder.AddListeningPort(m_serverAddress + ":" + m_serverPort, GrpcHelper::getServerCredentials(static_cast<grpcCredentials>(m_serverCredentials)));

    for (auto service: *m_services) {
        std::cout << "Registering IGrpcService # " << service->getServiceName() << std::endl;
        registerService(service);
    }

    if (m_receiveMessageMaxSize > 0) {
        m_builder.SetMaxReceiveMessageSize(m_receiveMessageMaxSize);
        std::cout << "Set max receive message size to: " << m_receiveMessageMaxSize << std::endl;
    }

    if (m_sendMessageMaxSize > 0) {
        m_builder.SetMaxSendMessageSize(m_sendMessageMaxSize);
        std::cout << "Set max send message size to: " << m_sendMessageMaxSize << std::endl;
    }

    std::unique_ptr<grpc::Server> server(m_builder.BuildAndStart());
    std::cout << "Server listening on " << m_serverAddress << ":" << m_serverPort << std::endl;
    server->Wait();
}

}}}
