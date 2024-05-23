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
			boost::json::object ping(const std::string& sender);
			boost::json::object connect(const std::string& sender, const std::string& id);
			boost::json::object disconnect(const std::string& sender);
			/*��*/
			boost::json::object marussia_request(const std::string& sender, const std::string& station_id, const boost::json::value& body);
			boost::json::object marussia_request(const std::string& sender, const std::string& station_ip, const std::string& body);
			boost::json::object mqtt_move(const std::string& sender, const std::string& station_id, const std::string& lift_block_id, int floor);
		}
		namespace response {
			/*�����*/
			boost::json::object ping(const std::string& sender);
			boost::json::object ping(const std::string& sender, const long& cpu_stat, const long& mem_stat,const boost::json::array& clients);
			boost::json::object errorTarget(const std::string& sender, const boost::json::value& target, ERROR_CODE err_code, const std::string& err_message = "");
			//������� success
			boost::json::object connect(const std::string& sender);
			boost::json::object disconnect(const std::string& sender);
			//������� � ����� ������
			boost::json::object connect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message = "");
			boost::json::object disconnect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message = "");

			/*marussia worker*/
			boost::json::object marussia_static_message(const std::string& sender, const std::string& station_id, const boost::json::object& response_body, const boost::json::object& session);
			boost::json::object marussia_mqtt_message(const std::string& sender, const std::string& station_id, const boost::json::object& response_body, const std::string& lift_block_id, int floor);
			/*mqtt worker*/
			boost::json::object mqtt_lift_move(const std::string& sender, const std::string& station_id, STATUS_OPERATION status, const std::string& err_message = "");
		}
		
	}
	namespace database {
		enum QUERY_METHOD {
			SELECT = 1
		};
		namespace request {
			boost::json::object ping(const std::string& sender);
			boost::json::object connect(const std::string& sender, const std::string& login, const std::string& password);
			boost::json::object disconnect(const std::string& sender);
			boost::json::object query(const std::string& sender, QUERY_METHOD method,const std::vector<std::string>& fields, const std::string& query);
		}
		namespace response {
			boost::json::object ping(const std::string& sender);
			boost::json::object errorTarget(const std::string& sender, const boost::json::value& target, ERROR_CODE err_code, const std::string& err_message = "");
			//������� success
			boost::json::object connect(const std::string& sender);
			boost::json::object disconnect(const std::string& sender);
			//������� � ����� ������
			boost::json::object connect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message = "");
			boost::json::object disconnect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message = "");
			boost::json::object query(const std::string& sender, QUERY_METHOD method,const std::map< std::string, std::vector<std::string> >& fields);
		}
	}
}