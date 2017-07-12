const bkyled = require('./build/Release/bky-led');

if (bkyled.setup(18, 12) == -1) {
  console.log('cannot setup serial led.');
  process.exit(1);
}

bkyled.setColor(0, 255, 0, 0);
bkyled.send();

setTimeout(() => {
  bkyled.cleanup();
}, 1000);
