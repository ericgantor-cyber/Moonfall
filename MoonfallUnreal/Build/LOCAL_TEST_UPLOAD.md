# Opt-in Moonfall UE5 Windows test uploader

Reports *small test summaries* to GitHub Actions; does not send raw logs, crash dumps, screenshots, saves, usernames, paths, or credentials.

## First-launch invitation

The first time Moonfall Build Center opens, it offers to enable optional automatic GitHub test summaries. **Yes** starts the existing setup script, which still requires GitHub CLI and an authenticated account; **No** leaves uploads disabled. Either answer dismisses future first-launch invitations, and the existing **AUTO TEST UPLOAD** button remains available to change the setting later. The invitation never appears in automated Build Center self-tests or when uploads are already installed. Merely running a game build never enables uploads.

## Build Center integration

Open the existing Moonfall Build Center and select **AUTO TEST UPLOAD**. The dialog first shows whether the scheduled uploader is currently installed. In the confirmation dialog, select **Yes** to enable five-minute reports, **No** to uninstall the uploader, or **Cancel** to leave settings alone. This is opt-in and is not triggered by normal builds. GitHub CLI must be installed and authenticated before enabling it.

## One-time setup on your Windows PC

1. Install GitHub CLI from https://cli.github.com/ and run gh auth login using an account authorized for ericgantor-cyber/Moonfall.
2. Update Moonfall after this PR is merged, or explicitly check out its branch.
3. Preview without uploading from the repository root (does not require GitHub login):
   powershell.exe -NoProfile -File .\MoonfallUnreal\Build\Install-LocalTestUpload.ps1 -TestNow
4. Enable automatic upload every five minutes:
   powershell.exe -NoProfile -File .\MoonfallUnreal\Build\Install-LocalTestUpload.ps1
5. Disable:
   powershell.exe -NoProfile -File .\MoonfallUnreal\Build\Install-LocalTestUpload.ps1 -Uninstall

Reads Moonfall-generated .log files in MoonfallUnreal/Saved/Logs, .log/.json in MoonfallUnreal/Saved/Automation/Reports, and .log/.err files from the existing local Build Center diagnostic folder (%LOCALAPPDATA%\\MoonfallBuildCenter\\Logs). A file must be unchanged for 60 seconds, nonempty, at most 10 MB and not a reparse point. Local SHA256 tracking in %LOCALAPPDATA%\Moonfall\TestUpload prevents repeated upload of identical content.

Automatic scans use an exclusive per-user lock. If an earlier scan is still in progress, a second invocation skips itself rather than dispatching duplicate reports. The lock is released when the process exits, including when an error occurs.

To prevent a large burst of GitHub Actions on first installation, only logs modified in the last seven days qualify, and each scan uploads at most ten new summaries and makes at most ten GitHub requests, even if uploads repeatedly fail. Newer logs are handled first; unsent eligible logs can be processed in subsequent scans.

Dispatch payload includes only outcome (passed/failed/unknown), source category, warning/error counts, and file digest. Status is heuristic, not proof of a successful native Windows build. GitHub workflow receipts appear after moonfall-local-test-receipt.yml exists on default branch. The existing GitHub monitoring task can detect receipt runs; it cannot see your actual gameplay or test results that never generate logs.

GitHub CLI remains responsible for authentication. No token is stored by the script. The Windows scheduled task runs as your signed-in user and may not execute if you are signed out. You must run the setup on your own PC; creating this PR alone cannot install anything there.

For automated regression testing, Upload-LocalTestResults.ps1 also accepts custom -GitHubCli and -CacheRoot arguments. These are optional; normal installation uses the logged-in gh CLI and a private per-user cache. CI simulates a failed dispatch, a successful retry, and a duplicate scan without accessing a real GitHub account.

**Corrupt upload history:** If `%LOCALAPPDATA%\Moonfall\TestUpload\sent.json` becomes unreadable, the uploader stops before sending new reports. The original history is preserved. Back it up and repair it before rerunning; removing it may cause previously sent logs to be reported again.
