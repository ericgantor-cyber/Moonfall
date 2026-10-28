// Moonfall Native per-user installer. Build: GOOS=windows GOARCH=amd64 go build ...
// The ZIP payload is appended to the PE with [uint64 payload length LE]["MFIN"].
// All installed files are from a hardcoded allowlist, never an arbitrary zip path.
package main

import (
 "archive/zip"
 "encoding/binary"
 "errors"
 "fmt"
 "io"
 "os"
 "os/exec"
 "path/filepath"
 "strings"
)
var version = "32.1"
var gameEXE = "MoonfallNative_v32_1.exe"
const magic = "MFIN"

func scriptQuote(s string) string { return "'" + strings.ReplaceAll(s, "'", "''") + "'" }
func installDir() (string,error) {
 base := os.Getenv("LOCALAPPDATA")
 if base=="" {return "", errors.New("LOCALAPPDATA environment variable not set")}
 return filepath.Join(base,"Programs","Moonfall Native"),nil
}
func runPowerShell(script string) error {
 c:=exec.Command("powershell.exe","-NoProfile","-NonInteractive","-Command",script)
 output,err:=c.CombinedOutput()
 if err!=nil {return fmt.Errorf("shortcut creation failed: %s: %w",strings.TrimSpace(string(output)),err)}
 return nil
}
func shortcuts(dest string) error {
 target:=filepath.Join(dest,gameEXE)
 uninstaller:=filepath.Join(dest,"Uninstall_Moonfall.exe")
 script:="$ErrorActionPreference='Stop';"+
  "$shell=New-Object -ComObject WScript.Shell;"+
  "$start=Join-Path ([Environment]::GetFolderPath('Programs')) 'Moonfall Native';"+
  "New-Item -ItemType Directory -Force -Path $start | Out-Null;"+
  "$desktop=Join-Path ([Environment]::GetFolderPath('Desktop')) 'Moonfall Native.lnk';"+
  "$menu=Join-Path $start 'Moonfall Native.lnk';"+
  "foreach($link in @($desktop,$menu)){ $s=$shell.CreateShortcut($link);"+
  "$s.TargetPath="+scriptQuote(target)+";"+
  "$s.WorkingDirectory="+scriptQuote(dest)+";"+
  "$s.IconLocation="+scriptQuote(target)+"; $s.Save() };"+
  "$u=$shell.CreateShortcut((Join-Path $start 'Uninstall Moonfall.lnk'));"+
  "$u.TargetPath="+scriptQuote(uninstaller)+"; $u.Save()"
 return runPowerShell(script)
}
func registerUninstaller(dest string) error {
 key:=`HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallNative`
 values:=map[string]string{
  "DisplayName":"Moonfall Native",
  "DisplayVersion":version,
  "Publisher":"Moonfall",
  "InstallLocation":dest,
  "DisplayIcon":filepath.Join(dest,gameEXE),
  "UninstallString":"\""+filepath.Join(dest,"Uninstall_Moonfall.exe")+"\"",
 }
 for name,value:=range values {
  c:=exec.Command("reg.exe","ADD",key,"/v",name,"/t","REG_SZ","/d",value,"/f")
  if o,err:=c.CombinedOutput();err!=nil{return fmt.Errorf("register %s: %s: %w",name,string(o),err)}
 }
 return nil
}
func extractPayload(exePath,dest string) error {
 f,err:=os.Open(exePath);if err!=nil{return err};defer f.Close()
 stat,err:=f.Stat();if err!=nil{return err}
 if stat.Size()<12{return errors.New("installer payload missing")}
 trailer:=make([]byte,12)
 if _,err=f.ReadAt(trailer,stat.Size()-12);err!=nil{return err}
 if string(trailer[8:])!=magic{return errors.New("installer payload signature missing")}
 size:=int64(binary.LittleEndian.Uint64(trailer[:8]))
 if size<=0 || size>stat.Size()-12{return errors.New("installer payload size invalid")}
 zr,err:=zip.NewReader(io.NewSectionReader(f,stat.Size()-12-size,size),size)
 if err!=nil{return err}
 allowed:=map[string]bool{gameEXE:true,"GODOT_LICENSE.txt":true,"Uninstall_Moonfall.exe":true}
 if err=os.MkdirAll(dest,0755);err!=nil{return err}
 seen:=make(map[string]bool)
 for _,entry:=range zr.File {
  if !allowed[entry.Name] || seen[entry.Name] || strings.ContainsAny(entry.Name,"/\\") {return fmt.Errorf("unexpected payload entry: %s",entry.Name)}
  seen[entry.Name]=true
  if entry.UncompressedSize64>250*1024*1024 {return errors.New("payload file too large")}
  reader,err:=entry.Open();if err!=nil{return err}
  target:=filepath.Join(dest,entry.Name)
  out,err:=os.OpenFile(target,os.O_CREATE|os.O_TRUNC|os.O_WRONLY,0644)
  if err!=nil{reader.Close();return fmt.Errorf("close Moonfall if already running: %w",err)}
  _,copyerr:=io.Copy(out,reader)
  cerr:=out.Close();reader.Close()
  if copyerr!=nil{return copyerr};if cerr!=nil{return cerr}
 }
 if len(seen)!=len(allowed){return errors.New("payload is incomplete")}
 return nil
}

func verifyPayload(exePath string) error {
 f,err:=os.Open(exePath);if err!=nil{return err};defer f.Close()
 stat,err:=f.Stat();if err!=nil{return err}
 if stat.Size()<12{return errors.New("missing installer payload")}
 tail:=make([]byte,12)
 if _,err=f.ReadAt(tail,stat.Size()-12);err!=nil{return err}
 if string(tail[8:])!=magic{return errors.New("invalid installer signature")}
 size:=int64(binary.LittleEndian.Uint64(tail[:8]))
 if size<=0 || size>stat.Size()-12{return errors.New("invalid archive size")}
 zr,err:=zip.NewReader(io.NewSectionReader(f,stat.Size()-12-size,size),size)
 if err!=nil{return err}
 allowed:=map[string]bool{gameEXE:true,"GODOT_LICENSE.txt":true,"Uninstall_Moonfall.exe":true}
 seen:=map[string]bool{}
 for _,file:=range zr.File {
  if !allowed[file.Name] || seen[file.Name] {return fmt.Errorf("unexpected file: %s",file.Name)}
  seen[file.Name]=true
  reader,err:=file.Open();if err!=nil{return err}
  _,err=io.Copy(io.Discard,reader)
  reader.Close();if err!=nil{return err}
 }
 if len(seen)!=len(allowed){return errors.New("missing payload file")}
 return nil
}

func main() {
 if len(os.Args)>1 && os.Args[1]=="--verify" {
  path,err:=os.Executable();if err==nil {err=verifyPayload(path)}
  if err!=nil {fmt.Fprintln(os.Stderr,"INVALID INSTALLER:",err);os.Exit(1)}
  fmt.Println("MOONFALL INSTALLER PAYLOAD PASS");return
 }
 dir,err:=installDir()
 if err==nil {
  path,err2:=os.Executable()
  if err2==nil {
   err=verifyPayload(path) // Reject incomplete or damaged payload before writing files.
   if err==nil {err=extractPayload(path,dir)}
  }else{err=err2}
 }
 if err==nil {err=shortcuts(dir)}
 if err==nil {err=registerUninstaller(dir)}
 if err!=nil {
  fmt.Fprintln(os.Stderr,"MOONFALL INSTALL FAILED:",err)
  fmt.Println("Press Enter to close.")
  fmt.Scanln()
  os.Exit(1)
 }
 fmt.Println("Moonfall Native "+version+" installed at:",dir)
 fmt.Println("Desktop and Start Menu shortcuts were created. Uninstall from Windows Apps settings.")
 fmt.Println("Press Enter to close.")
 fmt.Scanln()
}
