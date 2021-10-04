# iconv-dir
A library to convert gbk file names under some directory to utf8 file names or invert

可以将指定目录下的文件名编码转换为指定编码，目前支持gbk与utf8文件名编码之间的互转

---

## Examples
```sh
npm install iconv-dir
```

```javascript
const iconvDir = require('iconv-dir');
iconvDir.convertDirectory('gbk', 'utf-8', '/home/test', '.xls|.doc');
```

```typescript
import {convertDirectory} from 'iconv-dir';
convertDirectory('gbk', 'utf-8', '/home/test', '');
```
