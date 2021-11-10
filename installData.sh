# Download bag of words vocabulary
curl https://github.com/SolarFramework/SolARModuleFBOW/releases/download/fbowVocabulary/fbow_voc.zip -L -o fbow_voc.zip
mkdir -p data/fbow_voc
unzip -o fbow_voc.zip -d ./data/fbow_voc
rm fbow_voc.zip

# Download maps
curl https://artifact.b-com.com/solar-generic-local/maps/hololens/bcomLab/mapLabA_linux_0_10_0.zip -L -o mapA.zip
unzip -o mapA.zip -d ./data/maps
rm mapA.zip

curl https://artifact.b-com.com/solar-generic-local/maps/hololens/bcomLab/mapLabB_linux_0_10_0.zip -L -o mapB.zip
unzip -o mapB.zip -d ./data/maps
rm mapB.zip

# Download calibration file
echo Download calibration file
curl https://artifact.b-com.com/solar-generic-local/captures/hololens/hololens_calibration.json -L -o hololens_calibration.json
mkdir ./data/calibrations
mv hololens_calibration.json ./data/calibrations/hololens_calibration.json

