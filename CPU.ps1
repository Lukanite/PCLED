$port = new-Object System.IO.Ports.SerialPort COM3,9600,None,8,one
$port.ReadTimeout = 4000
While (1) {
$port.open()
$t = [int]((Get-WmiObject -Namespace "Root\OpenHardwareMonitor" -Query "SELECT Identifier,Value FROM Sensor WHERE Sensortype='Temperature' and (Identifier='/nvidiagpu/0/temperature/0' or Identifier='/atigpu/0/temperature/0')" | Measure-Object -Maximum -Property Value | Format-Wide -Property Maximum | out-string).Trim())
$port.ReadLine()
$port.Write([string]$t + "^" + [string]$t + "*")
Write-Host $t
$port.Close()
#Start-Sleep -s 1
}
