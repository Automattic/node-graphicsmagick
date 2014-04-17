var gm = require('./');
var fs = require('fs');

var buf = fs.readFileSync('test.jpg');
var img = gm.image(buf);

img.format('JPEG');
process.stdout.write(img.buffer);
