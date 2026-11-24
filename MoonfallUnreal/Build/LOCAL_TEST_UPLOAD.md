# Opt-in Moonfall UE5 Windows test uploader

Reports *small test summaries* to GitHub Actions; does not send raw logs, crash dumps, screenshots, saves, usernames, paths, or credentials.

## Build Center integration

Open the existing Moonfall Build Center and select **AUTO TEST UPLOAD**. In the confirmation dialog, select **Yes** to enable five-minute reports, **No** to uninstall the uploader, or **Cancel** to leave settings alone. This is opt-in and is not triggered by normal builds. GitHub CLI must be installed and authenticated before enabling it.

## One-time setup on your Windows PC

1. Install GitHub CLI from https://cli.github.com/ and run gh auth login using an account authorized for ericgantor-cyber/Moonfall.
2. Update Moonfall after this PR is merged, or explicitly check out its branch.
3. Preview without uploading from the repository root:
   powershell.exe -NoProfile -File .\MoonfallUnreal\Build\Install-LocalTestUpload.ps1 -TestNow
4. Enable automatic upload every five minutes:
   powershell.exe -NoProfile -File .\MoonfallUnreal\Build\Install-LocalTestUpload.ps1
5. Disable:
   powershell.exe -NoProfile -File .\MoonfallUnreal\Build\Install-LocalTestUpload.ps1 -Uninstall

Reads Moonfall-generated .log files in MoonfallUnreal/Saved/Logs, .log/.json in MoonfallUnreal/Saved/Automation/Reports, and .log/.err files from the existing local Build Center diagnostic folder (%LOCALAPPDATA%\\MoonfallBuildCenter\\Logs). A file must be unchanged for 60 seconds, nonempty, at most 10 MB and not a reparse point. Local SHA256 tracking in %LOCALAPPDATA%\Moonfall\TestUpload prevents repeated upload of identical content.

Dispatch payload includes only outcome (passed/failed/unknown), source category, warning/error counts, and file digest. Status is heuristic, not proof of a successful native Windows build. GitHub workflow receipts appear after moonfall-local-test-receipt.yml exists on default branch. The existing GitHub monitoring task can detect receipt runs; it cannot see your actual gameplay or test results that never generate logs.

GitHub CLI remains responsible for authentication. No token is stored by the script. The Windows scheduled task runs as your signed-in user and may not execute if you are signed out. You must run the setup on your own PC; creating this PR alone cannot install anything there.
