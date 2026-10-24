// Lightweight NVIDIA/Windows GPU diagnostics. No vendor SDK required.
'use strict';
const VENDORS = new Map([[0x10de, 'NVIDIA'], [0x1002, 'AMD'], [0x8086, 'Intel']]);
function formatGpuReport(features = {}, info = {}, accelerationEnabled = false) {
  const devices = Array.isArray(info.gpuDevice) ? info.gpuDevice : [];
  const active = devices.find(device => device.active) || devices[0];
  const vendor = active ? VENDORS.get(Number(active.vendorId)) || 'Other/Unknown' : 'Not reported';
  const status = key => typeof features[key] === 'string' ? features[key] : 'unknown';
  return [
    'Chromium hardware acceleration: ' + (accelerationEnabled ? 'enabled' : 'not confirmed'),
    'Reported active GPU vendor: ' + vendor,
    'GPU compositing: ' + status('gpu_compositing'),
    'WebGL: ' + status('webgl'),
    'WebGL 2: ' + status('webgl2'),
    'Canvas: ' + status('2d_canvas'),
    '',
    'This reports Chromium graphics capabilities, not whether the NVIDIA overlay detected Moonfall.',
    'Use the NVIDIA App for recording, GPU monitoring and driver settings.'
  ].join('\n');
}
module.exports = {formatGpuReport};
