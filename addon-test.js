// $ node-gyp configure build
// $ sudo node addon-test.js

const ledlib = require('./build/Release/serialled');

if (ledlib.setup(18, 12) == -1) {
  console.log('cannot setup serial led.');
  process.exit(1);
}

ledlib.setColor(0, 255, 0, 0);
ledlib.send();

setTimeout(() => {
  ledlib.cleanup();
}, 1000);
