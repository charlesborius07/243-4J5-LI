$arduinoPath = "C:\Users\user\AppData\Local\Programs\Arduino IDE\Arduino IDE.exe"
$sketchPath = "C:\Users\user\Documents\GitHub\llm-t-beam-supreme"
$fqbn = "esp32:esp32:esp32s3"
$port = "COM15"

& "$arduinoPath" --board "$fqbn" --port "$port" --upload "$sketchPath"