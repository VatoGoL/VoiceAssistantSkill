#include "JSONFormatter.hpp"
using namespace json_formatter;
using namespace boost;

json::object worker::request::ping(const std::string& sender) {
	return json::object({ { "sender", sender}, {"target", "ping"} });
}
json::object worker::request::connect(const std::string& sender, const std::string& id) {
	return json::object({ { "sender", sender}, {"target", "connect"}, {"request", {{"id", id}}} });
}
json::object worker::request::disconnect(const std::string& sender) {
	return json::object({ { "sender", sender}, {"target", "disconnect"} });
}

json::object worker::request::marussia_request(const std::string& sender, const std::string& station_id,const json::value& body) {
	return json::object({	{ "sender", sender},
							{ "target", "marussia_station_request"}, 
							{ "request", {
									{"station_id", station_id},
									{"body", body}
								}
							}
						});
}
json::object worker::request::marussia_request(const std::string& sender, const std::string& station_ip, const std::string& body) {
	return json::object({	{ "sender", sender},
							{ "target", "marussia_station_request"}, 
							{ "request", {
									{"station_ip", station_ip},
									{"body", body}
								}
							}
						});
}
json::object worker::request::mqtt_move(const std::string& sender, const std::string& station_id, const std::string& lift_block_id, int floor) {
	return json::object({ { "sender", sender},
							{ "target", "move_lift"},
							{ "request", {
									{"station_id", station_id},
									{"mqtt_command", {
										{"lb_id", lift_block_id},
										{"value", std::to_string(floor)}
										}
									}
								}
							}
						});
}
boost::json::object worker::response::ping(const std::string& sender)
{
	return json::object({ {"sender", sender}, {"target", "ping"}, { "response", {{"status", "success"}}} });
}
json::object worker::response::ping(const std::string& sender,const long& cpu_stat,const long& mem_stat,const boost::json::array& clients) {
	return json::object({ {"sender", sender}, {"target", "ping"}, { "response", {
																		{"status", "success"}, 
																		{"cpu", cpu_stat}, 
																		{"mem", mem_stat},
																		{"clients_ip", clients}
																	}} });
}
json::object worker::response::connect(const std::string& sender){
	return json::object({ {"sender", sender}, {"target", "connect"}, { "response", {{"status", "success"}}} });
}
json::object worker::response::disconnect(const std::string& sender){
	return json::object({ { "sender", sender}, {"target", "disconnect"}, { "response", {{"status", "success"}}} });
}

json::object worker::response::errorTarget(const std::string& sender,const boost::json::value& target, ERROR_CODE err_code, const std::string& err_message) {
	return json::object({	{"sender", sender},
							{"target", target},
							{"response", {
								{"status", "fail"},
								{"error_code", std::to_string(err_code)},
								{"message", err_message}
								}
							}
						});
}
json::object worker::response::connect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message){
	return json::object({	{"sender", sender}, 
							{"target", "connect"}, 
							{ "response", {
									{"status", "fail"},
									{"error_code", std::to_string(err_code)},
									{"message", err_message}
								}
							} 
						});
}
json::object worker::response::disconnect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message){
	return json::object({ {"sender", sender},
							{"target", "disconnect"},
							{ "response", {
									{"status", "fail"},
									{"error_code", std::to_string(err_code)},
									{"message", err_message}
								}
							}
						});
}
json::object worker::response::marussia_static_message(const std::string& sender, const std::string& station_id,const  json::object& response_body,const boost::json::object& session){
	return json::object({	{"sender", sender},
							{"target", "static_message"},
							{"response", {
								{"station_id", station_id},
								{"response_body", response_body},
								{"session", session},
								}
							}
						});
}
json::object worker::response::marussia_mqtt_message(const std::string& sender, const std::string& station_id,const json::object& response_body, const std::string& lift_block_id, int floor){
	return json::object({ {"sender", sender},
							{"target", "move_lift"},
							{"response", {
								{"station_id", station_id},
								{"response_body", response_body},
								{"mqtt_command", {
									{"lb_id", lift_block_id},
									{"value", std::to_string(floor)}
									}
								}
								}
							}
		});
}
json::object worker::response::mqtt_lift_move(const std::string& sender, const std::string& station_id, STATUS_OPERATION status, const std::string& err_message){
	
	switch (status) {
	case STATUS_OPERATION::success:
		return json::object({	{"sender", sender},
								{"target", "mqtt_message"},
								{"response", {
									{"station_id", station_id},
									{"status", "success"}
									}
								}
							});
		break;
	case STATUS_OPERATION::fail:
		return json::object({ {"sender", sender},
								{"target", "mqtt_message"},
								{"response", {
									{"station_id", station_id},
									{"status", "fail"},
									{"message", err_message},
									}
								}
			});
		break;
	default:
		return {};
		break;
	}
}


json::object database::request::ping(const std::string& sender){
	return json::object({ { "sender", sender}, {"target", "ping"} });
}
json::object database::request::connect(const std::string& sender, const std::string& login, const std::string& password){
	return json::object({	{"sender", sender}, 
							{"target", "connect"}, 
							{"request", {
								{"login", login},
								{"password", password},
								}
							}
						});
}
json::object database::request::disconnect(const std::string& sender){
	return worker::request::disconnect(sender);
}
json::object database::request::query(const std::string& sender, QUERY_METHOD method,const std::vector<std::string>& fields, const std::string& query){
	json::array json_fields;
	for (size_t i = 0, length = fields.size(); i < length; ++i) {
		json_fields.emplace_back(fields[i]);
	}
	switch (method) {
	case SELECT:
		return json::object({	{"sender", sender},
								{"target", "db_query"},
								{"request", {
									{"method", "select"},
									{"fields", json_fields},
									{"query", query}
									}
								}
							});
		break;
	default:
		return {};
	}
}

json::object database::response::errorTarget(const std::string& sender, const boost::json::value& target, ERROR_CODE err_code, const std::string& err_message) {
	return worker::response::errorTarget(sender, target, err_code, err_message);
}
json::object database::response::ping(const std::string& sender){
	return worker::response::ping(sender);
}
json::object database::response::connect(const std::string& sender){
	return worker::response::connect(sender);
}
json::object database::response::disconnect(const std::string& sender){
	return worker::response::disconnect(sender);
}
json::object database::response::connect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message){
	return worker::response::connect(sender, err_code, err_message);
}
json::object database::response::disconnect(const std::string& sender, ERROR_CODE err_code, const std::string& err_message){
	return worker::response::disconnect(sender, err_code, err_message);
}
json::object database::response::query(const std::string& sender, QUERY_METHOD method,const std::map< std::string, std::vector<std::string> >& fields){
	json::object result;
	json::object response;
	result["sender"] = sender;
	result["target"] = "db_query";
	switch (method) {
		case SELECT:
			response["method"] = "select";
			break;
	}
	json::array value;
	for (auto i = fields.begin(); i != fields.end(); i++) {
		value = {};
		for (size_t j = 0, length = (*i).second.size(); j < length; j++) {
			value.emplace_back((*i).second[j]);
		}
		response[(*i).first] = value;
	}
	result["response"] = response;
	return result;
}