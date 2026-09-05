#include "mkwii/nas_http.h"

#include <cassert>

int main() {
	const std::string response = mkwii::nas_connectivity_response();
	assert(response.find("HTTP/1.1 200 OK\r\n") == 0);
	assert(response.find("Content-Type: text/html\r\n") != std::string::npos);
	assert(response.find("Content-Length: 2\r\n") != std::string::npos);
	assert(response.find("X-Organization: Nintendo\r\n") != std::string::npos);
	assert(response.find("Server: BigIP\r\n") != std::string::npos);
	assert(response.ends_with("\r\n\r\nok"));

	const std::string login_request = "POST /ac HTTP/1.1\r\n"
									  "Host: naswii.nintendowifi.net\r\n\r\n"
									  "action=bG9naW4%2A&userid=test";
	const std::string login_response =
		mkwii::nas_response_for_request(login_request);
	assert(login_response.find("Content-Type: text/plain\r\n") !=
		   std::string::npos);
	assert(login_response.find("NODE: wifiappe1\r\n") != std::string::npos);
	assert(login_response.find(
			   "retry=MA**&returncd=MDAx&locator=Z2FtZXNweS5jb20*&") !=
		   std::string::npos);
	assert(login_response.find("&challenge=") != std::string::npos);
	assert(login_response.find("&token=TkRT") != std::string::npos);
	assert(login_response.find("&datetime=") != std::string::npos);
	assert(login_response.ends_with("\r\n"));
	assert(login_response.size() > response.size());

	const std::string sake_request =
		"POST /SakeStorageServer/StorageServer.asmx HTTP/1.1\r\n"
		"SOAPAction: \"http://gamespy.net/sake/GetMyRecords\"\r\n\r\n"
		"<GetMyRecords/>";
	const std::string sake_response =
		mkwii::nas_response_for_request(sake_request);
	assert(sake_response.find("Content-Type: text/xml; charset=utf-8\r\n") !=
		   std::string::npos);
	assert(sake_response.find("<GetMyRecordsResult>Success</GetMyRecordsResult>") !=
		   std::string::npos);
	assert(sake_response.find("<values/>") != std::string::npos);

	const std::string create_record_request =
		"POST /SakeStorageServer/StorageServer.asmx HTTP/1.1\r\n"
		"SOAPAction: \"http://gamespy.net/sake/CreateRecord\"\r\n\r\n"
		"<CreateRecord/>";
	const std::string create_record_response =
		mkwii::nas_response_for_request(create_record_request);
	assert(create_record_response.find(
			   "<CreateRecordResult>Success</CreateRecordResult>") !=
		   std::string::npos);
	assert(create_record_response.find("<recordid>1</recordid>") !=
		   std::string::npos);

	const std::string second_login_response =
		mkwii::nas_response_for_request(login_request);
	assert(second_login_response != login_response);
	assert(second_login_response.find("&challenge=") != std::string::npos);
	assert(second_login_response.find("&token=TkRT") != std::string::npos);

	assert(mkwii::nas_response_for_request("GET / HTTP/1.1\r\n\r\n") ==
		   response);
	return 0;
}