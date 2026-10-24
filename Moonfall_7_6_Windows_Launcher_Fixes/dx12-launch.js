// Opt-in launcher for the independent Direct3D 12 graphics experiment.
// This is NOT Moonfall's playable renderer and does not enable DLSS.
'use strict';
const path = require('node:path');
const fs = require('node:fs');
const {spawn} = require('node:child_process');

const EXE = 'MoonfallDX12Prototype.exe';
function prototypePath({isPackaged, resourcesPath, appDir}) {
  return isPackaged
    ? path.join(resourcesPath, 'dx12', EXE)
    : path.resolve(appDir, '..', 'native', 'dx12-prototype', 'build', 'Release', EXE);
}
function launchPrototype(executable, options = {}) {
  const fileExists = options.fileExists || fs.existsSync;
  const spawnChild = options.spawnChild || spawn;
  if (!fileExists(executable)) {
    throw new Error('DX12 demo is not installed. Reinstall Moonfall or compile native/dx12-prototype on Windows.');
  }
  // No shell or user-provided command; do not hand Electron's session to the child.
  const child = spawnChild(executable, [], {
    cwd: path.dirname(executable), stdio: 'ignore', windowsHide: false
  });
  return child;
}
module.exports = {EXE, prototypePath, launchPrototype};
