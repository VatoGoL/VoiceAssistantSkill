#pragma once
#include <string>
#include <vector>
#include <map>
#include <boost/json.hpp>

namespace json_formatter{

	enum ERROR_CODE {
		CONNECT = 3,
		DISCONNECT,
		TARGET
	};
	enum STATUS_OPERATION{
		success = 1,
		fail
	};
	namespace worker {
		namespace request {
			/*�����*/
			boost::json::object ping(std::string sender);
			boost::json::object connect(std::string sender, std::string id);
			boost::json::object disconnect(std::string sender);

			/*��*/
			boost::json::object marussia_request(std::string sender, std::string station_id, boost::json::value body);
			boost::json::object mqtt_move(std::string sender, std::string station_id, std::string lift_block_id, int floor);
		}
		namespace response {
			/*�����*/
			boost::json::object ping(std::string sender);
			boost::json::object errorTarget(std::string sender, boost::json::value target, ERROR_CODE err_code, std::string err_message = "");
			//������� success
			boost::json::object connect(std::string sender);
			boost::json::object disconnect(std::string sender);
			//������� � ����� ������
			boost::json::object connect(std::string sender, ERROR_CODE err_code, std::string err_message = "");
			boost::json::object disconnect(std::string sender, ERROR_CODE err_code, std::string err_message = "");

			/*marussia worker*/
			boost::json::object marussia_static_message(std::string sender, std::string station_id, boost::json::object response_body);
			boost::json::object marussia_mqtt_message(std::string sender, std::string station_id, boost::json::object response_body, std::string lift_block_id, int floor);
			/*mqtt worker*/
			boost::json::object mqtt_lift_move(std::string sender, std::string station_id, STATUS_OPERATION status, std::string err_message = "");
		}
		
	}
	namespace database {
		enum QUERY_METHOD {
			SELECT = 1
		};
		namespace request {
			boost::json::object ping(std::string sender);
			boost::json::object connect(std::string sender, std::string login, std::string password);
			boost::json::object disconnect(std::string sender);
			boost::json::object query(std::string sender, QUERY_METHOD method, std::vector<std::string> fields, std::string query);
		}
		namespace response {
			boost::json::object ping(std::string sender);
			boost::json::object errorTarget(std::string sender, boost::json::value target, ERROR_CODE err_code, std::string err_message = "");
			//������� success
			boost::json::object connect(std::string sender);
			boost::json::object disconnect(std::string sender);
			//������� � ����� ������
			boost::json::object connect(std::string sender, ERROR_CODE err_code, std::string err_message = "");
			boost::json::object disconnect(std::string sender, ERROR_CODE err_code, std::string err_message = "");
			boost::json::object query(std::string sender, QUERY_METHOD method, std::map< std::string, std::vector<std::string> > fields);
		}
	}
}