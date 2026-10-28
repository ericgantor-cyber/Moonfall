// Per-user Moonfall uninstaller. Removes only the known Moonfall install directory.
package main
import (
 "fmt"
 "os"
 "os/exec"
 "path/filepath"
 "strings"
)
func psq(s string)string{return "'"+strings.ReplaceAll(s,"'","''")+"'"}
func main(){
 local:=os.Getenv("LOCALAPPDATA")
 if local==""{fmt.Fprintln(os.Stderr,"LOCALAPPDATA missing");os.Exit(1)}
 dest:=filepath.Join(local,"Programs","Moonfall Native")
 // A detached cleanup waits for this uninstaller to exit before deleting its own file.
 script:="$ErrorActionPreference='SilentlyContinue'; Start-Sleep -Seconds 2;"+
  "$start=Join-Path ([Environment]::GetFolderPath('Programs')) 'Moonfall Native';"+
  "Remove-Item -LiteralPath (Join-Path ([Environment]::GetFolderPath('Desktop')) 'Moonfall Native.lnk') -Force;"+
  "Remove-Item -LiteralPath $start -Recurse -Force;"+
  "Remove-Item -LiteralPath "+psq(dest)+" -Recurse -Force;"+
  "& reg.exe DELETE 'HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MoonfallNative' /f | Out-Null;"
 cmd:=exec.Command("powershell.exe","-NoProfile","-NonInteractive","-WindowStyle","Hidden","-Command",script)
 if err:=cmd.Start();err!=nil{fmt.Fprintln(os.Stderr,"Could not start cleanup:",err);os.Exit(1)}
 fmt.Println("Moonfall uninstall requested. The game directory and shortcuts will be removed.")
}
