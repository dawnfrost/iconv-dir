#include <napi.h>
#include <cstdio>
#include <string.h>
#include <memory.h>
#include <locale.h>
#include <string>
#include <vector>

char defaultCodePage[128] = { 0 };


std::vector<std::string>  split(const std::string& str, const std::string& delim) {
	std::vector<std::string> res;
	if (str.empty()) {
		return  res;
	}
	if (delim.empty()) {
		res.push_back(str);
		return res;
	}

	std::string strs = str + delim;
	size_t pos;
	size_t size = strs.size();

	for (size_t i = 0; i < size; ++i) {
		pos = strs.find(delim, i);
		if (pos < size) {
			std::string s = strs.substr(i, pos - i);
			res.push_back(s);
			i = pos + delim.size() - 1;
		}
	}

	return res;
}

bool isGBKString(bool& isAllAscii, char const* str, int strMaxLength = 4000) {
	unsigned char const* iterator = (unsigned char const*)str;
	// is utf-8 bom
	if (iterator[0] == 0xEF && iterator[1] == 0xBB && iterator[2] == 0xBF) {
		isAllAscii = false;
		return false;
	}

	// is utf-32 big endian
	if (iterator[0] == 0 && iterator[1] == 0 && iterator[2] == 0xFE && iterator[3] == 0xFF) {
		isAllAscii = false;
		return false;
	}

	// is utf-32 little endian
	if (iterator[0] == 0xFF && iterator[1] == 0xFE && iterator[2] == 0 && iterator[3] == 0) {
		isAllAscii = false;
		return false;
	}

	// is utf-16 big endian
	if (iterator[0] == 0xFE || iterator[1] == 0xFF) {
		isAllAscii = false;
		return false;
	}

	// is utf-16 little endian
	if (iterator[0] == 0xFF || iterator[1] == 0xFE) {
		isAllAscii = false;
		return false;
	}

	// gbk bom
	if (iterator[0] == 0x84 && iterator[1] == 0x31 && iterator[2] == 0x95 && iterator[3] == 0x33) {
		iterator += 4;
	}

	isAllAscii = true;
	unsigned char temp = *iterator++;
	while (temp && strMaxLength-- > 0) {
		if ((temp & 0x80) == 0) {
			temp = *iterator++;
			continue;
		}

		if (temp >= 0x81 && temp <= 0xFE) {
			unsigned char temp2 = *iterator++;
			if (temp2 >= 0x40 && temp2 <= 0xFE) {
				temp = *iterator++;
				isAllAscii = false;
			}
			else {
				isAllAscii = false;
				return false;
			}
		}
		else {
			isAllAscii = false;
			return false;
		}
	}

	return true;
}

bool isUTF8String(bool& isAllAscii, char const* str, int strMaxLength = 4000) {
	unsigned char const prefix7 = 0xFD;
	unsigned char const prefix6 = 0xFC;
	unsigned char const prefix5 = 0xF8;
	unsigned char const prefix4 = 0xF0;
	unsigned char const prefix3 = 0xE0;
	unsigned char const prefix2 = 0xC0;

	unsigned char const* iterator = (unsigned char const*)str;

	// gbk bom
	if (iterator[0] == 0x84 && iterator[1] == 0x31 && iterator[2] == 0x95 && iterator[3] == 0x33) {
		isAllAscii = false;
		return false;
	}

	// is utf-32 big endian
	if (iterator[0] == 0 && iterator[1] == 0 && iterator[2] == 0xFE && iterator[3] == 0xFF) {
		isAllAscii = false;
		return false;
	}

	// is utf-32 little endian
	if (iterator[0] == 0xFF && iterator[1] == 0xFE && iterator[2] == 0 && iterator[3] == 0) {
		isAllAscii = false;
		return false;
	}

	// is utf-16 big endian
	if (iterator[0] == 0xFE || iterator[1] == 0xFF) {
		isAllAscii = false;
		return false;
	}

	// is utf-16 little endian
	if (iterator[0] == 0xFF || iterator[1] == 0xFE) {
		isAllAscii = false;
		return false;
	}

	// is utf-8 bom
	if (iterator[0] == 0xEF && iterator[1] == 0xBB && iterator[2] == 0xBF) {
		iterator += 3;
	}

	isAllAscii = true;
	unsigned char temp = *iterator++;
	int remainingBytes = 0;
	while (temp && strMaxLength-- > 0) {
		if (remainingBytes > 0) {
			if ((temp & 0xC0) == 0x80) {
				remainingBytes--;
				temp = *iterator++;
				continue;
			}
			else {
				isAllAscii = false;
				return false;
			}
		}
		else {
			if ((temp & 0x80) == 0) {
				temp = *iterator++;
				continue;
			}
		}

		if (temp >= prefix6 && temp <= prefix7) {
			remainingBytes = 5;
			temp = *iterator++;
			isAllAscii = false;
		}
		else if (temp >= prefix5) {
			remainingBytes = 4;
			temp = *iterator++;
			isAllAscii = false;
		}
		else if (temp >= prefix4) {
			remainingBytes = 3;
			temp = *iterator++;
			isAllAscii = false;
		}
		else if (temp >= prefix3) {
			remainingBytes = 2;
			temp = *iterator++;
			isAllAscii = false;
		}
		else if (temp >= prefix2) {
			remainingBytes = 1;
			temp = *iterator++;
			isAllAscii = false;
		}
		else {
			isAllAscii = false;
			return false;
		}
	}

	return true;
}

#ifdef WIN32

#include <Windows.h>
#include <wchar.h>

void determineDefaultCodePage() {
	switch (GetACP()) {
	case 936:
		strncpy_s(defaultCodePage, "GB2312", sizeof(defaultCodePage) - 1);
		break;
	case 950:
		strncpy_s(defaultCodePage, "big5", sizeof(defaultCodePage) - 1);
		break;
	case 1200:
		strncpy_s(defaultCodePage, "UTF-16", sizeof(defaultCodePage) - 1);
		break;
	case 12000:
		strncpy_s(defaultCodePage, "UTF-32", sizeof(defaultCodePage) - 1);
		break;
	case 1201:
		strncpy_s(defaultCodePage, "unicodeFFFE", sizeof(defaultCodePage) - 1);
		break;
	case 12001:
		strncpy_s(defaultCodePage, "32BE", sizeof(defaultCodePage) - 1);
		break;
	case 54936:
		strncpy_s(defaultCodePage, "GB18030", sizeof(defaultCodePage) - 1);
		break;
	case 28591:
		strncpy_s(defaultCodePage, "iso-8859-1", sizeof(defaultCodePage) - 1);
		break;
	case 28592:
		strncpy_s(defaultCodePage, "iso-8859-2", sizeof(defaultCodePage) - 1);
		break;
	case 28593:
		strncpy_s(defaultCodePage, "iso-8859-3", sizeof(defaultCodePage) - 1);
		break;
	case 28594:
		strncpy_s(defaultCodePage, "iso-8859-4", sizeof(defaultCodePage) - 1);
		break;
	case 28595:
		strncpy_s(defaultCodePage, "iso-8859-5", sizeof(defaultCodePage) - 1);
		break;
	case 28596:
		strncpy_s(defaultCodePage, "iso-8859-6", sizeof(defaultCodePage) - 1);
		break;
	case 28597:
		strncpy_s(defaultCodePage, "iso-8859-7", sizeof(defaultCodePage) - 1);
		break;
	case 28598:
		strncpy_s(defaultCodePage, "iso-8859-8", sizeof(defaultCodePage) - 1);
		break;
	case 28599:
		strncpy_s(defaultCodePage, "iso-8859-9", sizeof(defaultCodePage) - 1);
		break;
	case 28603:
		strncpy_s(defaultCodePage, "iso-8859-13", sizeof(defaultCodePage) - 1);
		break;
	case 28605:
		strncpy_s(defaultCodePage, "iso-8859-15", sizeof(defaultCodePage) - 1);
		break;
	case 65000:
		strncpy_s(defaultCodePage, "UTF-7", sizeof(defaultCodePage) - 1);
		break;
	case 65001:
		strncpy_s(defaultCodePage, "UTF-8", sizeof(defaultCodePage) - 1);
		break;
	default:
		strncpy_s(defaultCodePage, "GBK", sizeof(defaultCodePage) - 1);
		break;
	}
}

int getCodePage(char const* cp) {
	if (_strcmpi(cp, "gb2312") == 0 || _strcmpi(cp, "gb-2312") == 0 || _strcmpi(cp, "gbk") == 0) {
		return 936;
	}
	else if (_strcmpi(cp, "big5") == 0) {
		return 950;
	}
	else if (_strcmpi(cp, "utf-16") == 0 || _strcmpi(cp, "utf16") == 0) {
		return 1200;
	}
	else if (_strcmpi(cp, "utf-32") == 0 || _strcmpi(cp, "utf32") == 0) {
		return 12000;
	}
	else if (_strcmpi(cp, "unicodeFFFE") == 0) {
		return 1201;
	}
	else if (_strcmpi(cp, "32BE") == 0) {
		return 12001;
	}
	else if (_strcmpi(cp, "GB18030") == 0 || _strcmpi(cp, "GB-18030") == 0) {
		return 54936;
	}
	else if (_strcmpi(cp, "iso-8859-1") == 0) {
		return 28591;
	}
	else if (_strcmpi(cp, "iso-8859-2") == 0) {
		return 28592;
	}
	else if (_strcmpi(cp, "iso-8859-3") == 0) {
		return 28593;
	}
	else if (_strcmpi(cp, "iso-8859-4") == 0) {
		return 28594;
	}
	else if (_strcmpi(cp, "iso-8859-5") == 0) {
		return 28595;
	}
	else if (_strcmpi(cp, "iso-8859-6") == 0) {
		return 28596;
	}
	else if (_strcmpi(cp, "iso-8859-7") == 0) {
		return 28597;
	}
	else if (_strcmpi(cp, "iso-8859-8") == 0) {
		return 28598;
	}
	else if (_strcmpi(cp, "iso-8859-9") == 0) {
		return 28599;
	}
	else if (_strcmpi(cp, "iso-8859-13") == 0) {
		return 28603;
	}
	else if (_strcmpi(cp, "iso-8859-15") == 0) {
		return 28605;
	}
	else if (_strcmpi(cp, "utf-7") == 0 || _strcmpi(cp, "utf7") == 0) {
		return 65000;
	}
	else if (_strcmpi(cp, "utf-8") == 0 || _strcmpi(cp, "utf8") == 0) {
		return 65001;
	}
	else {
		return CP_ACP;
	}
}

bool isUTF8CodePage(char const* codePage) {
	return _strcmpi(codePage, "utf8") == 0 || _strcmpi(codePage, "utf-8") == 0;
}

bool isGBKCodePage(char const* codePage) {
	return _strcmpi(codePage, "gbk") == 0 || _strcmpi(codePage, "gb2312") == 0 || _strcmpi(codePage, "gb-2312") == 0 || _strcmpi(codePage, "gb18030") == 0 || _strcmpi(codePage, "gb-18030") == 0;
}


int convertCodePage(char const* fromCodePage, char const* toCodePage, char const* src, size_t srcLength, char* dest, size_t destLength) {
	int fromCP = getCodePage(fromCodePage);
	int toCP = getCodePage(toCodePage);
	int tempLength = MultiByteToWideChar(fromCP, 0, src, (int)srcLength, NULL, NULL);
	wchar_t* temp = new wchar_t[tempLength];
	int ret = MultiByteToWideChar(fromCP, 0, src, (int)srcLength, temp, tempLength);
	if (ret < 0) {
		delete[] temp;
		return ret;
	}

	ret = WideCharToMultiByte(toCP, 0, temp, tempLength, dest, (int)destLength, NULL, NULL);
	delete[] temp;
	return ret;
}

bool endsWith(std::string const& str, std::string const& end) {
	size_t i = str.rfind(end);
	return (i != std::string::npos) && (i == (str.length() - end.length()));
}

int convertDirectoryFilenames(char const* fromCodePage, char const* toCodePage, char const* dir, char const* ext = NULL) {
	WIN32_FIND_DATAA findData;
	HANDLE handle;
	char fullName[2048] = { 0 };
	char filePathName[2048] = { 0 };
	strncpy_s(filePathName, dir, sizeof(filePathName));
	int len = (int)strlen(filePathName);
	bool hasSeparator = false;
	if (filePathName[len - 1] == '\\') {
		strcat_s(filePathName, "*.*");
		hasSeparator = true;
	}
	else {
		strcat_s(filePathName, "\\*.*");
		hasSeparator = false;
	}

	std::string extName;
	if (ext && ext[0]) {
		extName = ext;
	}

	std::vector<std::string> exts = split(extName, "|");

	char tempFilename[MAX_PATH * 4] = { 0 };
	char destFilename[MAX_PATH * 8] = { 0 };
	bool fromGBK = isGBKCodePage(fromCodePage);
	bool fromUTF8 = isUTF8CodePage(fromCodePage);
	bool toGBK = isGBKCodePage(toCodePage);
	bool toUTF8 = isUTF8CodePage(toCodePage);
	handle = FindFirstFileA(filePathName, &findData);
	int ret = -1;
	if (handle != INVALID_HANDLE_VALUE) {
		ret = 0;
		while (FindNextFileA(handle, &findData)) {
			if (_strcmpi(findData.cFileName, ".") == 0 || _strcmpi(findData.cFileName, "..") == 0)
			{
				continue;
			}

			std::string strName = findData.cFileName;
			if (!exts.empty()) {
				bool matched = false;
				for (std::vector<std::string>::const_iterator i = exts.begin(); i != exts.end(); ++i) {
					if (endsWith(strName, *i)) {
						matched = true;
						break;
					}
				}
				if (!matched) {
					continue;
				}
			}

			bool isAllAscii = false;
			if ((fromGBK && toUTF8 && isGBKString(isAllAscii, strName.c_str())) || (fromUTF8 && toGBK && isUTF8String(isAllAscii, strName.c_str()))) {
				if (!isAllAscii) {
					convertCodePage(fromCodePage, toCodePage, findData.cFileName, strName.length(), tempFilename, sizeof(tempFilename));
					if (hasSeparator) {
						sprintf_s(fullName, sizeof(fullName), "%s%s", dir, strName.c_str());
						sprintf_s(destFilename, sizeof(destFilename), "%s%s", dir, tempFilename);
					}
					else {
						sprintf_s(fullName, sizeof(fullName), "%s\\%s", dir, strName.c_str());
						sprintf_s(destFilename, sizeof(destFilename), "%s\\%s", dir, tempFilename);
					}
					if (rename(fullName, destFilename) == 0) {
						ret++;
					}
				}
			}
		}
	}

	return ret;
}

#else
#include <dirent.h>
#include <langinfo.h>
#include <iconv.h>

void determineDefaultCodePage() {
	setlocale(LC_ALL, "");
	char* locstr = setlocale(LC_CTYPE, NULL);
	char* encoding = nl_langinfo(CODESET);
	printf("Locale is %s\n", locstr);
	printf("Encoding is %s\n", encoding);
	strncpy(defaultCodePage, encoding, sizeof(defaultCodePage) - 1);
}

bool isUTF8CodePage(char const* codePage) {
	return strcasecmp(codePage, "utf8") == 0 || strcasecmp(codePage, "utf-8") == 0;
}

bool isGBKCodePage(char const* codePage) {
	return strcasecmp(codePage, "gbk") == 0 || strcasecmp(codePage, "gb2312") == 0 || strcasecmp(codePage, "gb-2312") == 0 || strcasecmp(codePage, "gb18030") == 0 || strcasecmp(codePage, "gb-18030") == 0;
}

int convertCodePage(char const* fromCodePage, char const* toCodePage, char const* input, size_t inputLength, char* output, size_t outputLength)
{
	iconv_t cd;
	char** pin = (char**)&input;
	char** pout = &output;

	cd = iconv_open(toCodePage, fromCodePage);
	if (cd == 0) return -1;
	memset(output, 0, outputLength);
	size_t ret = iconv(cd, pin, &inputLength, pout, &outputLength);
	iconv_close(cd);
	return (int)ret;
}

bool endsWith(std::string const& str, std::string const& end) {
	size_t i = str.rfind(end);
	return (i != std::string::npos) && (i == (str.length() - end.length()));
}

int convertDirectoryFilenames(char const* fromCodePage, char const* toCodePage, char const* dir, char const* ext = NULL) {
	DIR* pDir;
	struct dirent* d;
	pDir = opendir(dir);

	int len = (int)strlen(dir);
	bool hasSeparator = false;
	if (dir[len - 1] == '/') {
		hasSeparator = true;
	}
	else {
		hasSeparator = false;
	}

	char filePathName[2048] = { 0 };
	char tempFilename[1024] = { 0 };
	char destPathName[2048] = { 0 };
	bool fromGBK = isGBKCodePage(fromCodePage);
	bool fromUTF8 = isUTF8CodePage(fromCodePage);
	bool toGBK = isGBKCodePage(toCodePage);
	bool toUTF8 = isUTF8CodePage(toCodePage);

	std::string extName;
	if (ext && ext[0]) {
		extName = ext;
	}

	std::vector<std::string> exts = split(extName, "|");
	int ret = 0;
	while ((d = readdir(pDir)))
	{
		if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
			continue;
		}

		std::string strName = d->d_name;
		if (!exts.empty()) {
			bool matched = false;
			for (std::vector<std::string>::const_iterator i = exts.begin(); i != exts.end(); ++i) {
				if (endsWith(strName, *i)) {
					matched = true;
					break;
				}
			}
			if (!matched) {
				continue;
			}
		}

		bool isAllAscii = false;
		if ((fromGBK && toUTF8 && isGBKString(isAllAscii, strName.c_str())) || (fromUTF8 && toGBK && isUTF8String(isAllAscii, strName.c_str()))) {
			if (!isAllAscii) {
				convertCodePage(fromCodePage, toCodePage, strName.c_str(), strName.length(), tempFilename, sizeof(tempFilename));
				if (hasSeparator) {
					sprintf(filePathName, "%s%s", dir, strName.c_str());
					sprintf(destPathName, "%s%s", dir, tempFilename);
				}
				else {
					sprintf(filePathName, "%s/%s", dir, strName.c_str());
					sprintf(destPathName, "%s/%s", dir, tempFilename);
				}
				if (rename(filePathName, destPathName) == 0) {
					ret++;
				}
			}
		}
	}

	closedir(pDir);
	return ret;
}

#endif


Napi::Value ConvertDirectory(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	std::string fromCodePage = info[0].As<Napi::String>().Utf8Value();
	std::string toCodePage = info[1].As<Napi::String>().Utf8Value();
	std::string dir = info[2].As<Napi::String>().Utf8Value();
	std::string ext = info[3].As<Napi::String>().Utf8Value();

	int ret = convertDirectoryFilenames(fromCodePage.c_str(), toCodePage.c_str(), dir.c_str(), ext.c_str());
	Napi::Number num = Napi::Number::New(env, ret);
	return num;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "convDir"), Napi::Function::New(env, ConvertDirectory));
	return exports;
}

// 固定的宏使用
NODE_API_MODULE(addon, Init)

