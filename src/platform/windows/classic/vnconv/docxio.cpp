#include "stdafx.h"
#include "vnconv.h"
#include "docxio.h"
#include "miniz.h"
#include <string>
#include <vector>

std::wstring Utf8ToUtf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
    std::wstring utf16(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &utf16[0], size_needed);
    return utf16;
}

std::string Utf16ToUtf8(const std::wstring& utf16) {
    if (utf16.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &utf16[0], (int)utf16.size(), NULL, 0, NULL, NULL);
    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &utf16[0], (int)utf16.size(), &utf8[0], size_needed, NULL, NULL);
    return utf8;
}

std::string ProcessXmlTextNode(const std::string& text, int inCharset, int outCharset) {
    if (text.empty()) return text;
    
    std::wstring utf16Text = Utf8ToUtf16(text);
    
    bool inIsUnicode = (inCharset == CONV_CHARSET_UNICODE || inCharset == CONV_CHARSET_UNIDECOMPOSED || inCharset == CONV_CHARSET_WINCP1258 || inCharset == CONV_CHARSET_UNIUTF8);
    bool outIsUnicode = (outCharset == CONV_CHARSET_UNICODE || outCharset == CONV_CHARSET_UNIDECOMPOSED || outCharset == CONV_CHARSET_WINCP1258 || outCharset == CONV_CHARSET_UNIUTF8);

    std::vector<BYTE> inBuf;
    int inLenBytes = 0;
    int actualInCharset = inCharset;

    if (inIsUnicode) {
        inBuf.assign((BYTE*)utf16Text.data(), (BYTE*)(utf16Text.data() + utf16Text.length()));
        inLenBytes = utf16Text.length() * 2;
        actualInCharset = CONV_CHARSET_UNICODE;
    } else {
        inBuf.resize(utf16Text.length());
        for (size_t i = 0; i < utf16Text.length(); i++) {
            inBuf[i] = (BYTE)(utf16Text[i] & 0xFF);
        }
        inLenBytes = inBuf.size();
    }

    int maxOutLen = inLenBytes * 4 + 10;
    std::vector<BYTE> outBuf(maxOutLen);
    int actualOutCharset = outIsUnicode ? CONV_CHARSET_UNICODE : outCharset;

    int ret = VnConvert(actualInCharset, actualOutCharset, inBuf.data(), outBuf.data(), inLenBytes, maxOutLen);

    if (ret == VNCONV_NO_ERROR && maxOutLen > 0) {
        std::wstring outUtf16;
        if (outIsUnicode) {
            outUtf16.assign((wchar_t*)outBuf.data(), maxOutLen / 2);
        } else {
            outUtf16.resize(maxOutLen);
            for (int i = 0; i < maxOutLen; i++) {
                outUtf16[i] = (wchar_t)outBuf[i];
            }
        }
        return Utf16ToUtf8(outUtf16);
    }
    
    return text;
}

std::string ConvertXmlText(const std::string& xml, int inCharset, int outCharset)
{
	std::string result;
	result.reserve(xml.size());
	
	size_t pos = 0;
	while (pos < xml.size()) {
		size_t nextOpen = xml.find('<', pos);
		if (nextOpen == std::string::npos) {
			result.append(ProcessXmlTextNode(xml.substr(pos), inCharset, outCharset));
			break;
		} else {
			result.append(ProcessXmlTextNode(xml.substr(pos, nextOpen - pos), inCharset, outCharset));
			
			size_t nextClose = xml.find('>', nextOpen);
			if (nextClose == std::string::npos) {
				result.append(xml.substr(nextOpen));
				break;
			} else {
				result.append(xml.substr(nextOpen, nextClose - nextOpen + 1));
				pos = nextClose + 1;
			}
		}
	}
	return result;
}

int DocxOdtConvert(int inCharset, int outCharset, const TCHAR *inFile, const TCHAR *outFile)
{
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));
	
#ifdef _UNICODE
	char inFileAnsi[MAX_PATH];
	char outFileAnsi[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, inFile, -1, inFileAnsi, MAX_PATH, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, outFile, -1, outFileAnsi, MAX_PATH, NULL, NULL);
	if (!mz_zip_reader_init_file(&zip_archive, inFileAnsi, 0)) return VNCONV_ERR_INPUT_FILE;
#else
	if (!mz_zip_reader_init_file(&zip_archive, inFile, 0)) return VNCONV_ERR_INPUT_FILE;
#endif

	mz_zip_archive zip_writer;
	memset(&zip_writer, 0, sizeof(zip_writer));
	
#ifdef _UNICODE
	if (!mz_zip_writer_init_file(&zip_writer, outFileAnsi, 0)) {
#else
	if (!mz_zip_writer_init_file(&zip_writer, outFile, 0)) {
#endif
		mz_zip_reader_end(&zip_archive);
		return VNCONV_ERR_OUTPUT_FILE;
	}

	int num_files = mz_zip_reader_get_num_files(&zip_archive);
	for (int i = 0; i < num_files; i++) {
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;

		if (strcmp(file_stat.m_filename, "word/document.xml") == 0 || 
		    strcmp(file_stat.m_filename, "content.xml") == 0) {
			
			size_t uncomp_size;
			void *p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
			if (p) {
				std::string xmlStr((const char*)p, uncomp_size);
				mz_free(p);
				
				std::string converted = ConvertXmlText(xmlStr, inCharset, outCharset);
				
				mz_zip_writer_add_mem(&zip_writer, file_stat.m_filename, converted.data(), converted.size(), MZ_DEFAULT_COMPRESSION);
			} else {
				mz_zip_writer_add_from_zip_reader(&zip_writer, &zip_archive, i);
			}
		} else {
			mz_zip_writer_add_from_zip_reader(&zip_writer, &zip_archive, i);
		}
	}

	mz_zip_writer_finalize_archive(&zip_writer);
	mz_zip_writer_end(&zip_writer);
	mz_zip_reader_end(&zip_archive);

	return VNCONV_NO_ERROR;
}
