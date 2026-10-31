using System.Diagnostics;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Text.Json;

namespace MoonfallLauncher;

internal sealed record UpdateInfo(string Version, string DownloadUrl, string Sha256, string ReleaseUrl, long Size);

internal static class MoonfallUpdates
{
    private const long MaximumInstallerBytes = 12L * 1024 * 1024 * 1024;
    private const string LatestRelease = "https://api.github.com/repos/ericgantor-cyber/Moonfall/releases/latest";
    private static readonly HttpClient Client = CreateClient();

    private static HttpClient CreateClient()
    {
        var client = new HttpClient { Timeout = TimeSpan.FromMinutes(15) };
        client.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("MoonfallLauncher", "1.0"));
        client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
        return client;
    }

    internal static async Task<UpdateInfo?> CheckAsync(CancellationToken cancellation = default)
    {
        using var response = await Client.GetAsync(LatestRelease, cancellation);
        response.EnsureSuccessStatusCode();
        using var data = JsonDocument.Parse(await response.Content.ReadAsStreamAsync(cancellation));
        var root = data.RootElement;
        if (root.TryGetProperty("draft", out var draft) && draft.GetBoolean()) return null;
        if (root.TryGetProperty("prerelease", out var prerelease) && prerelease.GetBoolean()) return null;
        var tag = root.GetProperty("tag_name").GetString() ?? "";
        // Only strictly numeric stable tags are eligible, e.g. v1.2.3 or v1.2.3.4.
        var numericTag = tag.TrimStart('v', 'V');
        if (numericTag.Length == 0 || numericTag.Split('.').Length is < 2 or > 4 ||
            numericTag.Split('.').Any(part => part.Length == 0 || !part.All(char.IsAsciiDigit)) ||
            !Version.TryParse(numericTag, out var remoteVersion)) return null;
        var current = typeof(Program).Assembly.GetName().Version ?? new Version(0, 1, 0);
        if (remoteVersion <= current) return null;

        foreach (var asset in root.GetProperty("assets").EnumerateArray())
        {
            var name = asset.GetProperty("name").GetString() ?? "";
            if (!name.Equals("MoonfallUnreal-Setup.exe", StringComparison.OrdinalIgnoreCase)) continue;
            if (!asset.TryGetProperty("size", out var assetSize) || assetSize.GetInt64() <= 0 || assetSize.GetInt64() > MaximumInstallerBytes) continue;
            var digest = asset.TryGetProperty("digest", out var value) ? value.GetString() ?? "" : "";
            if (!digest.StartsWith("sha256:", StringComparison.OrdinalIgnoreCase)) continue;
            var hash = digest["sha256:".Length..];
            if (hash.Length != 64 || !hash.All(Uri.IsHexDigit)) continue;
            var url = asset.GetProperty("browser_download_url").GetString() ?? "";
            var releaseUrl = root.GetProperty("html_url").GetString() ?? "";
            if (!Uri.TryCreate(url, UriKind.Absolute, out var uri) ||
                uri.Scheme != Uri.UriSchemeHttps || uri.Host != "github.com") continue;
            return new UpdateInfo(tag, url, hash, releaseUrl, assetSize.GetInt64());
        }
        // Do not offer unverified installers or a release without the exact Unreal installer.
        return null;
    }

    internal static async Task<string> DownloadVerifiedAsync(UpdateInfo update, IProgress<long>? progress = null, CancellationToken cancellation = default)
    {
        var dir = Path.Combine(Path.GetTempPath(), "MoonfallUpdater");
        Directory.CreateDirectory(dir);
        var destination = Path.Combine(dir, "MoonfallUnreal-Setup-" + Guid.NewGuid().ToString("N") + ".exe");
        try
        {
            using (var response = await Client.GetAsync(update.DownloadUrl, HttpCompletionOption.ResponseHeadersRead, cancellation))
            {
                response.EnsureSuccessStatusCode();
                if (response.Content.Headers.ContentLength is long length && length > MaximumInstallerBytes)
                    throw new InvalidDataException("Update installer is larger than the allowed download limit.");
                await using var input = await response.Content.ReadAsStreamAsync(cancellation);
                await using var output = File.Create(destination);
                var buffer = new byte[1024 * 128];
                long written = 0;
                int count;
                while ((count = await input.ReadAsync(buffer, cancellation)) > 0)
                {
                    written += count;
                    progress?.Report(written);
                    if (written > MaximumInstallerBytes)
                        throw new InvalidDataException("Update installer exceeded the allowed download limit.");
                    await output.WriteAsync(buffer.AsMemory(0, count), cancellation);
                }
            }
            await using var stream = File.OpenRead(destination);
            var calculated = Convert.ToHexString(await SHA256.HashDataAsync(stream, cancellation));
            if (!calculated.Equals(update.Sha256, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Downloaded installer failed SHA-256 verification.");
            return destination;
        }
        catch
        {
            if (File.Exists(destination)) File.Delete(destination);
            throw;
        }
    }
}
