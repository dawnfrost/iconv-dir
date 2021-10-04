'use strict';

const fs = require('fs');
const path = require('path');

function dropDir(p) {
  const list = fs.readdirSync(p)
  list.forEach((v, i) => {
    const url = path.join(p, v);
    const stats = fs.statSync(url);
    if (stats.isFile()) {
      fs.unlinkSync(url);
    } else {
      dropDir(url);
    }
  });
  fs.rmdirSync(p);
}

const vendor = path.join(__dirname, "./vendor");
const dll = path.join(__dirname, "./build/Release/iconv-dir.node");
if (fs.existsSync(vendor)) {
  dropDir(vendor);
}
fs.mkdirSync(vendor);
fs.copyFileSync(dll, path.join(vendor, "iconv-dir.node"));
dropDir(path.join(__dirname, "./build"));
