@echo off
:: Download bag of words vocabulary
echo Download bag of word dictionnaries
curl https://github.com/SolarFramework/SolARModuleFBOW/releases/download/fbowVocabulary/fbow_voc.zip -L -o fbow_voc.zip
echo Unzip bag of word dictionnaries
powershell Expand-Archive fbow_voc.zip -DestinationPath .\data\fbow_voc -F
del fbow_voc.zip

:: Download maps
echo Download and install maps
curl https://artifact.b-com.com/solar-generic-local/maps/hololens/bcomLab/mapLabA_win_0_10_0.zip -L -o mapA.zip
powershell Expand-Archive mapA.zip -DestinationPath .\data\maps -F
del mapA.zip

curl https://artifact.b-com.com/solar-generic-local/maps/hololens/bcomLab/mapLabB_win_0_10_0.zip -L -o mapB.zip
powershell Expand-Archive mapB.zip -DestinationPath .\data\maps -F
del mapB.zip

:: Download calibration file
echo Download calibration file
curl https://artifact.b-com.com/solar-generic-local/captures/hololens/hololens_calibration.yml -L -o hololens_calibration.yml
md .\data\calibrations
move hololens_calibration.yml .\data\calibrations\hololens_calibration.yml
