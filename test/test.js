var fs = require('fs');
var os = require('os');
var gm = require('../');
var path = require('path');
var assert = require('assert');
var exec = require('child_process').exec;

var testJpegFilename = path.resolve(__dirname, 'fixtures', 'test.jpg');
var testJpegBuffer = fs.readFileSync(testJpegFilename);

describe('GraphicsMagick', function () {

  describe('.image(Buffer)', function () {

    it('should return an "Image" instance', function () {
      var img = gm.image(testJpegBuffer);
      assert.equal(img.constructor.name, 'Image');
    });

  });

  describe('Image', function () {

    it('should output format "JPEG"', function (done) {
      var img = gm.image(testJpegBuffer);
      img.format('JPEG');
      var tmpFile = path.resolve(os.tmpDir(), 'test.jpg');
      after(function (done) {
        fs.unlink(tmpFile, done);
      });
      fs.writeFile(tmpFile, img.buffer, function (err) {
        if (err) return done(err);
        exec('file "' + tmpFile + '"', function (err, stdout) {
          if (err) return done(err);
          assert(/jpe?g/i.test(stdout.replace(tmpFile, '')));
          done();
        });
      });
    });

    it('should output format "PNG"', function (done) {
      var img = gm.image(testJpegBuffer);
      img.format('PNG');
      var tmpFile = path.resolve(os.tmpDir(), 'test.png');
      after(function (done) {
        fs.unlink(tmpFile, done);
      });
      fs.writeFile(tmpFile, img.buffer, function (err) {
        if (err) return done(err);
        exec('file "' + tmpFile + '"', function (err, stdout) {
          if (err) return done(err);
          assert(/png/i.test(stdout.replace(tmpFile, '')));
          done();
        });
      });
    });

  });

});
