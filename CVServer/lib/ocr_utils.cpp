#include "ocr_utils.hpp"

using namespace std;
using namespace Json;

void init_dictionary(unordered_set<string> &dictionary) {
	ifstream infile("./words/words");
	string currWord;
	while (infile >> currWord) {
		// cout << currWord << endl;
		dictionary.insert(currWord);
	}
}

bool isEmpty(char ch) {
	return ch == ' ' || ch == '?' || ch == '\n';
}

bool isEmptyOrSingleCharacter(string st, int i) {
	if (isEmpty(st[i])) 
		return true;
	bool leftIsEmpty = i == 0 ? true : isEmpty(st[i - 1]);
	bool rightIsEmpty = i == st.length() - 1? true : isEmpty(st[i + 1]);
	return leftIsEmpty && rightIsEmpty;
}

bool isValidWord(unordered_set<string> dictionary, string st) {
	return st.length() >= 3 && dictionary.find(st) != dictionary.end();
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string getBase64EncodingofImg(Mat img) {
	vector<uchar> buf;
	imencode(".jpg", img, buf);
	uchar *enc_msg = new uchar[buf.size()];
	for (int i = 0; i < buf.size(); i++) {
		enc_msg[i] = buf[i];
	}
	return base64_encode(enc_msg, buf.size());
}

string parseOCRResponse(string content) {
    CharReaderBuilder builder;
    StreamWriterBuilder writeBuilder;
    CharReader * reader = builder.newCharReader();
    Value root;
	string text, errors;
    bool parsingSuccessful = reader->parse(content.c_str(), content.c_str() + content.size(), &root, &errors);
    delete reader;
    if ( !parsingSuccessful ) { cout << errors << endl; }
    const Value line = root["responses"][0]["fullTextAnnotation"]["text"];
	writeBuilder.settings_["indentation"] = "";
	text = writeString(writeBuilder, line);
    return regex_replace(text, regex(R"(\\n)"), " ");;
}

string parseSDResponse(string content, vector<double> &screen_box) {
    CharReaderBuilder builder;
    StreamWriterBuilder writeBuilder;
    CharReader * reader = builder.newCharReader();
    Value root;
	string errors;
    bool parsingSuccessful = reader->parse(content.c_str(), content.c_str() + content.size(), &root, &errors);
    delete reader;
    if ( !parsingSuccessful ) { cout << errors << endl; }
    const Value isScreenValue = root["isScreen"];
    const Value topValue = root["Top"];
    const Value leftValue = root["Left"];
    const Value widthValue = root["Width"];
    const Value heightValue = root["Height"];
	writeBuilder.settings_["indentation"] = "";
	string isScreen = writeString(writeBuilder, isScreenValue);
	screen_box[0] = stod(writeString(writeBuilder, topValue));
	screen_box[1] = stod(writeString(writeBuilder, leftValue));
	screen_box[2]= stod(writeString(writeBuilder, widthValue));
	screen_box[3] = stod(writeString(writeBuilder, heightValue));
    return isScreen;
}

void getScreenRegionFromImage(Mat img, string &screen_exist, vector<double> &screen_box) {
	// Base64 encoding
	string curlBase64 = getBase64EncodingofImg(img);
	// cout << "Encoded base64: " << curlBase64 << endl;

	// Send CURL request to Google Vision API
	String content;
	CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/rest_apis/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "cache-control: no-cache");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlBase64.c_str());
    // curl_easy_setopt(hnd, CURLOPT_VERBOSE, 0L);
    CURLcode ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // Parse JSON response
    screen_exist = parseSDResponse(content, screen_box);
}

void getOCRTextFromImage(Mat img, string &fulltext) {

	// Base64 encoding
	string curlBase64 = getBase64EncodingofImg(img);
	// cout << "Encoded base64: " << curlBase64 << endl;

	// Send CURL request to Google Vision API
	String content;
	string curlStr = "{\n  \"requests\": [\n    {\n      \"image\": {\n        \"content\": \"" + curlBase64 + "\"\n      },\n      \"features\": [\n        {\n          \"type\": \"TEXT_DETECTION\",\n          \"maxResults\": 1\n        }\n      ],\n      \"imageContext\": {\n        \"languageHints\": [\n          \"en\"\n        ]\n      }\n    }\n  ]\n}";
	CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_URL, "https://vision.googleapis.com/v1/images:annotate?key=");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlStr.c_str());
    // curl_easy_setopt(hnd, CURLOPT_VERBOSE, 0L);
    CURLcode ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // Parse JSON response
    string text = parseOCRResponse(content);

    // Pass full string back for now
    fulltext = text;

    // Saved for future use

}

float ocr_similarity_score(string str1, string str2) {
	int len1 = str1.length();
	int len2 = str2.length();
	int LCS[len1 + 1][len2 + 1];
	for (int i1 = 0; i1 <= len1; i1++) {
		for (int i2 = 0; i2 <= len2; i2++) {
			if (i1 == 0 || i2 == 0) {
				LCS[i1][i2] = 0;
				continue;
			}
			if (isEmptyOrSingleCharacter(str1, i1 - 1)) {
				LCS[i1][i2] = LCS[i1 - 1][i2];
				continue;
			}
			if (isEmptyOrSingleCharacter(str2, i2 - 1)) {
				LCS[i1][i2] = LCS[i1][i2 - 1];
				continue;
			}
			if (str1[i1 - 1] == str2[i2 - 1]) {
				LCS[i1][i2] = LCS[i1 - 1][i2 - 1] + 1;
				continue;
			}
			LCS[i1][i2] = LCS[i1 - 1][i2] > LCS[i1][i2 - 1] ? LCS[i1 - 1][i2] : LCS[i1][i2 - 1];
		}
	}
	// int len = (len1 + len2) / 2; 
	// int len = len1 < len2 ? len1 : len2;
	// return LCS[len1][len2] / (float)len;
	return LCS[len1][len2];
}