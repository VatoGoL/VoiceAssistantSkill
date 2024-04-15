#include "JSONFormatter.hpp"
using namespace json_formatter;
using namespace boost;

json::object worker::request::ping(std::string sender) {
	return json::object({ { "sender", sender}, {"target", "ping"} });
}
json::object worker::request::connect(std::string sender, std::string id) {
	return json::object({ { "sender", sender}, {"target", "connect"}, {"request", {{"id", id}}} });
}
json::object worker::request::disconnect(std::string sender) {
	return json::object({ { "sender", sender}, {"target", "disconnect"} });
}
json::object worker::request::marussia_request(std::string sender, std::string station_id, json::value body) {
	return json::object({	{ "sender", sender},
							{ "target", "marussia_station_request"}, 
							{ "request", {
									{"station_id", station_id},
									{"body", body}
								}
							}
						});
}
json::object worker::request::mqtt_move(std::string sender, std::string station_id, std::string lift_block_id, int floor) {
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

json::object worker::response::ping(std::string sender) {
	return json::object({ {"sender", sender}, {"target", "ping"}, { "response", {{"status", "success"}}} });
}
json::object worker::response::connect(std::string sender){
	return json::object({ {"sender", sender}, {"target", "connect"}, { "response", {{"status", "success"}}} });
}
json::object worker::response::disconnect(std::string sender){
	return json::object({ { "sender", sender}, {"target", "disconnect"}, { "response", {{"status", "success"}}} });
}
json::object worker::response::errorTarget(std::string sender, boost::json::value target, ERROR_CODE err_code, std::string err_message) {
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
json::object worker::response::connect(std::string sender, ERROR_CODE err_code, std::string err_message){
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
json::object worker::response::disconnect(std::string sender, ERROR_CODE err_code, std::string err_message){
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
json::object worker::response::marussia_static_message(std::string sender, std::string station_id, json::object response_body){
	return json::object({	{"sender", sender},
							{"target", "static_message"},
							{"response", {
								{"station_id", station_id},
								{"response_body", response_body}
								}
							}
						});
}
json::object worker::response::marussia_mqtt_message(std::string sender, std::string station_id, json::object response_body, std::string lift_block_id, int floor){
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
json::object worker::response::mqtt_lift_move(std::string sender, std::string station_id, STATUS_OPERATION status, std::string err_message){
	
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


json::object database::request::ping(std::string sender){
	return json::object({ { "sender", sender}, {"target", "ping"} });
}
json::object database::request::connect(std::string sender, std::string login, std::string password){
	return json::object({	{"sender", sender}, 
							{"target", "connect"}, 
							{"request", {
								{"login", login},
								{"password", password},
								}
							}
						});
}
json::object database::request::disconnect(std::string sender){
	return worker::request::disconnect(sender);
}
json::object database::request::query(std::string sender, QUERY_METHOD method, std::vector<std::string> fields, std::string query){
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

json::object database::response::errorTarget(std::string sender, boost::json::value target, ERROR_CODE err_code, std::string err_message) {
	return worker::response::errorTarget(sender, target, err_code, err_message);
}
json::object database::response::ping(std::string sender){
	return worker::response::ping(sender);
}
json::object database::response::connect(std::string sender){
	return worker::response::connect(sender);
}
json::object database::response::disconnect(std::string sender){
	return worker::response::disconnect(sender);
}
json::object database::response::connect(std::string sender, ERROR_CODE err_code, std::string err_message){
	return worker::response::connect(sender, err_code, err_message);
}
json::object database::response::disconnect(std::string sender, ERROR_CODE err_code, std::string err_message){
	return worker::response::disconnect(sender, err_code, err_message);
}
json::object database::response::query(std::string sender, QUERY_METHOD method, std::map< std::string, std::vector<std::string> > fields){
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