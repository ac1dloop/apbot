#pragma once

#include <iostream>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <td/telegram/Client.h>
#include <thread>
#include <functional>
#include <map>
#include <vector>
#include <deque>
#include <future>
#include <optional>
#include <unordered_map>
#include <array>
#include <sstream>

namespace APP {

	//Compile time constants
	namespace CONST {

		constexpr auto pidorbot_id = 446851068;
		constexpr auto pidorbot_message = "\xd0\xb8\xd0\xb4\xd0\xb8\x20\xd0\xbd\xd0\xb0\x20\xd1\x85\xd1\x83\xd0\xb9!!!"; //Иди на хуй на юникоде
		constexpr auto update_interval = 10;

	} //CONST

	struct Bot {

		//all classes inherited from Object in this lib
		using Object = td::td_api::object_ptr<td::td_api::Object>;
		using Update = td::td_api::object_ptr<td::td_api::Update>;
		using Request = td::td_api::object_ptr<td::td_api::Function>;
		using Error = td::td_api::object_ptr<td::td_api::error>;
		using Response = td::ClientManager::Response;
		using ReplyMessage = td::td_api::object_ptr<td::td_api::sendMessage>;
		using TdString = td::td_api::object_ptr<td::td_api::formattedText>;
		using CommandHandler = std::function<TdString(TdString)>;
		using inputMessage = td::td_api::object_ptr<td::td_api::message>;

		using ResponseHandler = std::function<void(Object)>;
		using ClientId = td::ClientManager::ClientId;
		using MessageSender = td::td_api::object_ptr <td::td_api::MessageSender>;
		using BotId = td::td_api::int53;
		using ChatId = td::td_api::int53;
		using UserId = td::td_api::int53;

		struct Settings {

			explicit Settings(int api_id, std::string api_hash) :api_id(api_id), api_hash(api_hash) {}

			//tdlib settings
			int api_id = 0;
			std::string api_hash = "";
			std::string db_dir = "tdlib";
			std::string files_dir = "dl";
			std::string system_lang = "en";
			std::string app_ver = "1.0";
			std::string device_model = "Raspberry pi";
			std::string system_version = "Linux";

		} settings;

		explicit Bot(int api_id, std::string api_hash) :settings{ api_id, api_hash } {

			td::ClientManager::execute(td::td_api::make_object<td::td_api::setLogVerbosityLevel>(0));

			client_id = client_manager->create_client_id();

			make_request(td::td_api::make_object<td::td_api::getOption>("version"), [this](Object obj) {
					auto optionValue = td::td_api::move_object_as<td::td_api::optionValueString>(obj);
					std::cout << "tdlib version " << optionValue->value_ << '\n';
			});

			auto txt = make_formatted_string(u8"Иди на хуй!!!");

			known_pidors[CONST::pidorbot_id] = std::move(txt);
		}

		~Bot() {}

		//main loop
		void run() {

			for (;;) {

				Response resp = client_manager->receive(CONST::update_interval);

				//no updates
				if (resp.object == nullptr)
					continue;

				//check error
				if (resp.object->get_id() == td::td_api::error::ID) {

					process_error(td::move_tl_object_as<td::td_api::error>(resp.object));

					continue;
				}

				//known response
				if (resp.request_id) {

					process_response(std::move(resp));

					continue;
				}

				//update
				process_update(td::td_api::move_object_as<td::td_api::Update>(resp.object));
			}

		}

	private:

		void process_error(Error err) {
			std::cout << "Error" << to_string(err) << std::endl;
		}

		void process_response(Response resp) {
			auto it = handlers.find(resp.request_id);
			
			if (it != handlers.end()) {
				it->second(std::move(resp.object));
				handlers.erase(it);//may be not optimal
			}
		}

		void process_auth_update(td::td_api::object_ptr<td::td_api::AuthorizationState> state) {
			using namespace td::td_api;

			switch (state->get_id())
			{
			case authorizationStateWaitTdlibParameters::ID:
			{
				std::cout << "authorizationStateWaitTdlibParameters" << std::endl;
				auto parameters = make_object<tdlibParameters>();
				parameters->database_directory_ = settings.db_dir;
				parameters->files_directory_ = settings.files_dir; //downloads folder
				parameters->use_file_database_ = true;
				parameters->use_chat_info_database_ = true;
				parameters->use_message_database_ = true;
				parameters->api_id_ = settings.api_id;
				parameters->api_hash_ = settings.api_hash;
				parameters->system_language_code_ = settings.system_lang;
				parameters->device_model_ = settings.device_model;
				parameters->application_version_ = settings.app_ver;
				parameters->system_version_ = settings.system_version;
				parameters->enable_storage_optimizer_ = true;
				make_request(make_object<setTdlibParameters>(std::move(parameters)));
				return;
			}
			case authorizationStateWaitEncryptionKey::ID:
			{
				std::cout << "authorizationStateWaitEncryptionKey" << std::endl;
				std::cout << "setting empty key by default" << std::endl;
				make_request(make_object<checkDatabaseEncryptionKey>(""));
				return;
			}
			case authorizationStateWaitPhoneNumber::ID:
			{
				std::cout << "authorizationStateWaitPhoneNumber" << std::endl;
				std::cout << "Enter phone number: " << std::flush;
				std::string phone_number;
				std::getline(std::cin, phone_number);
				make_request(make_object<setAuthenticationPhoneNumber>(phone_number, nullptr));
				return;
			}
			case authorizationStateWaitCode::ID:
			{
				std::cout << "authorizationStateWaitCode" << std::endl;
				std::cout << "Enter authentication code: " << std::flush;
				std::string code;
				std::getline(std::cin, code);
				make_request(make_object<checkAuthenticationCode>(std::move(code)));
				return;
			}
			case authorizationStateWaitOtherDeviceConfirmation::ID:
			{
				auto confirmation = td::td_api::move_object_as<td::td_api::authorizationStateWaitOtherDeviceConfirmation>(state);

				std::cout << "authorizationStateWaitOtherDeviceConfirmation" << std::endl;
				std::cout << "Confirm this login link on another device: " << confirmation->link_ << std::endl;

				return;
			}
			case authorizationStateWaitRegistration::ID:
			{
				std::cout << "authorizationStateWaitRegistration" << std::endl;
				std::string first_name;
				std::string last_name;
				std::cout << "Enter your first name: " << std::flush;
				std::cin >> first_name;
				std::cout << "Enter your last name: " << std::flush;
				std::cin >> last_name;
				make_request(make_object<registerUser>(first_name, last_name));
				return;
			}
			case authorizationStateWaitPassword::ID:
			{
				std::cout << "authorizationStateWaitPassword" << std::endl;
				std::cout << "Enter authentication password: " << std::flush;
				std::string password;
				std::getline(std::cin, password);
				make_request(make_object<checkAuthenticationPassword>(password));
				return;
			}
			case authorizationStateReady::ID:
			{
				std::cout << "authorizationStateReady" << std::endl;
				return;
			}
			case authorizationStateLoggingOut::ID:
			{
				std::cout << "authorizationStateLoggingOut" << std::endl;
				return;
			}
			case authorizationStateClosing::ID:
			{
				std::cout << "authorizationStateClosing" << std::endl;
				return;
			}
			case authorizationStateClosed::ID:
			{
				std::cout << "authorizationStateClosed" << std::endl;
				return;
			}
			default:
				std::cout << "unknown auth state" << std::endl;
				break;
			}
		}

		void process_update(td::td_api::object_ptr<td::td_api::Update> update) {
			//even not every update is used. switch covers all important updates
			switch (update->get_id())
			{
			case td::td_api::updateAuthorizationState::ID:
			{
				auto state = td::td_api::move_object_as<td::td_api::updateAuthorizationState>(update);

				process_auth_update(std::move(state->authorization_state_));

				return;
			}
			case td::td_api::updateUser::ID:
			{
				auto user = td::td_api::move_object_as<td::td_api::user>(static_cast<td::td_api::updateUser*>(update.get())->user_);

				//new information will override current user if exist
				known_users[user->id_] = std::move(user);

				return;
			}
			//the most important update
			case td::td_api::updateNewMessage::ID:
			{
				auto m = td::td_api::move_object_as<td::td_api::message>(static_cast<td::td_api::updateNewMessage*>(update.get())->message_);

				//sent by bot
				if (m->is_outgoing_)
					return;

				process_input_message(std::move(m));
			}
			default:
				//nothing to do
				break;
			}
		}

		void process_input_message(td::td_api::object_ptr<td::td_api::message> message) {
			auto sender = td::td_api::move_object_as<td::td_api::MessageSender>(message->sender_id_);

			//message from chat
			if (sender->get_id() != td::td_api::messageSenderUser::ID)
				return;

			//get id
			auto sender_user = td::td_api::move_object_as<td::td_api::messageSenderUser>(sender);

			//handle pidor
			if (known_pidors.find(sender_user->user_id_) != known_pidors.end()) {
				auto reply = make_formatted_string(known_pidors[sender_user->user_id_]->text_);

				replyToMesageWithText(std::move(message), std::move(reply));
			}

			//now we care about content
			auto message_content = std::move(message->content_);

			//can process only text
			if (message_content->get_id() != td::td_api::messageText::ID)
				return;

			auto text_content = td::td_api::move_object_as<td::td_api::messageText>(message_content);
			auto form_text = std::move(text_content->text_);

			//command starts with '/'
			if (form_text->text_.at(0) == '/') {

				auto known_user = known_users.find(sender_user->user_id_);

				if (known_user == known_users.end())
					return;

				if (!known_user->second->is_contact_)
					return replyToMesageWithText(std::move(message), make_formatted_string(u8"мы не знакомы"));

				auto command_text = copy_command_name(form_text);

				auto cmd_handler = command_handlers.find(command_text);

				if (cmd_handler == command_handlers.end())
					return replyToMesageWithText(std::move(message), make_formatted_string(u8"нет такой команды"));

				auto res_text = cmd_handler->second(std::move(form_text));

				return replyToMesageWithText(std::move(message), std::move(res_text));
			}

			//else append to pseudohistory
			text_messages[text_messages_index % text_messages.size()] = std::move(form_text);
			text_messages_index++;
		}

		void make_request(Request req, ResponseHandler handler) {
			client_manager->send(client_id, ++request_id, std::move(req));
			handlers.emplace(request_id, handler);
		}

		void make_request(Request req) {
			client_manager->send(client_id, ++request_id, std::move(req));
		}

		void replyToMesageWithText(inputMessage msg, TdString str){
			//send message to pidor
			auto send_message = td::td_api::make_object<td::td_api::sendMessage>();

			send_message->reply_to_message_id_ = msg->id_;
			send_message->chat_id_ = msg->chat_id_;

			//message settings
			//auto message_settings = td::td_api::make_object<td::td_api::messageSendOptions>();
			//message_settings->disable_notification_ = true;
			//message_settings->from_background_ = true;
			//message_settings->protect_content_ = true;
			//send_message->options_ = std::move(message_settings);

			//actual content of message
			auto message_content = td::td_api::make_object<td::td_api::inputMessageText>();
			message_content->text_ = std::move(str);

			send_message->input_message_content_ = std::move(message_content);

			make_request(std::move(send_message));
		}

		bool delPidor(UserId id) {
			auto it = known_pidors.find(id);

			if (it != known_pidors.end())
				known_pidors.erase(it);

			return false; //because 'nothing to do' is correct result
		}

		bool delPidor(const std::string& username) {
			for (const auto& [id, user] : known_users) {
				if (user->username_ == username)
					return delPidor(id);
			}

			return false;
		}

		std::string copy_command_name(TdString& str) {
			std::string& res(str->text_);

			return res.substr(1, res.find(' '));
		}

		TdString make_formatted_string(std::string str) {
			TdString res = td::td_api::make_object<td::td_api::formattedText>();

			res->text_ = std::move(str);

			return std::move(res);
		}

		//Api
		std::unique_ptr<td::ClientManager> client_manager{ std::make_unique<td::ClientManager>() };

		ClientId client_id{ 0 };

		uint64_t request_id{ 0 };

		std::map<uint64_t, ResponseHandler> handlers;

		//Cache
		std::unordered_map<ChatId, td::td_api::object_ptr<td::td_api::chat> > known_chats;
		std::unordered_map<UserId, td::td_api::object_ptr<td::td_api::user> > known_users;

		//Logic
		std::unordered_map<UserId, TdString> known_pidors;

		std::unordered_map<std::string, CommandHandler> command_handlers
		{
		{"help", [this](TdString str) {
			std::stringstream ss;

			ss << "help - get help\n";
			ss << "setloglevel - flood console\n";
			ss << "addpidor_id [id] [text] - add pidor to hatelist\n";
			ss << "addpidor_name [username] [text]\n";
			ss << "delpidor_id\n";
			ss << "delpidor_name\n";
			ss << "listpidors\n";

			return make_formatted_string(ss.str());
		}},
		{"setloglevel",[this](TdString str){
				int lvl;

				TdString res{ make_formatted_string("failed") };

				auto& s = str->text_;

				try {
					lvl = std::stoi(s.substr(s.find(' ') + 1, std::string::npos));
				}
				catch (...) {
					return res;
				}

				td::ClientManager::execute(td::td_api::make_object<td::td_api::setLogVerbosityLevel>(lvl));

				res->text_ = "success";
				return res;
		}},
		{"addpidor_id",[this](TdString str) {
			TdString res = make_formatted_string("failed");

			std::string& s = str->text_;

			auto fsp = s.find(' ');
			auto fsp2 = s.find(' ', fsp + 1);

			auto& second = s.substr(fsp + 1, fsp2 - fsp - 1);
			auto& last = s.substr(fsp2 + 1, std::string::npos);

			UserId id;

			try {
				id = std::stoll(second);
			}
			catch (...) {
				return res;
			}

			known_pidors[id] = make_formatted_string(last);

			res->text_ = "success";
			return res;
		}},
		{"addpidor_name",[this](TdString str) {
			TdString res = make_formatted_string("failed");

			std::string& s = str->text_;

			auto fsp = s.find(' ');
			auto fsp2 = s.find(' ', fsp + 1);

			auto& name = s.substr(fsp + 1, fsp2 - fsp - 1);
			auto& pidor_text = s.substr(fsp2 + 1, std::string::npos);

			UserId uid = 0;

			for (const auto& [id, user] : known_users) {
				if (user->username_ == name) {
					uid = id;
					break;
				}
			}

			if (uid == 0)
				return res;

			known_pidors[uid] = make_formatted_string(pidor_text);
			res->text_ = "success";
			return res;
		}},
		{"delpidor_id",[this](TdString str) {
			TdString res = make_formatted_string("failed");

			std::string& s = str->text_;

			UserId id;

			try {
				id = std::stoll(s.substr(s.find(' ') + 1, std::string::npos));
			}
			catch (...) {
				return res;
			}

			res->text_ = "success";
			return res;
		}},
		{"delpidor_name",[this](TdString str) {
			TdString res = make_formatted_string("failed");

			std::string& s = str->text_;

			UserId uid;

			auto& name = s.substr(s.find(' ') + 1, std::string::npos);

			for (const auto& [id, user] : known_users) {
				if (user->username_ == name) {
					uid = id;
					break;
				}
			}

			if (uid == 0)
				return res;

			auto pidor_it = known_pidors.find(uid);

			if (pidor_it != known_pidors.end())
				known_pidors.erase(pidor_it);

			res->text_ = "success";
			return res;
		}},
		{"listpidors",[this](TdString str) {
				std::stringstream ss;

				int i{1};
				for (const auto& [id, text] : known_pidors) {
					auto user_it = known_users.find(id);

					ss << i << ':';

					if (user_it != known_users.end()) {
						ss << user_it->second->username_;
					}
					else {
						ss << id;
					}

					ss << " [" << text->text_ << "]\n";
				}

				return make_formatted_string(ss.str());
		}},
		{"listcontacts",[this](TdString str) {
			std::stringstream ss;

			int i{ 1 };
			for (const auto& [id, user] : known_users) {
				if (user->is_contact_)
					ss << i++ << ": " << user->username_ << " " << user->id_ << '\n';
			}

			return make_formatted_string(ss.str());
		}},
		{"listknownusers",[this](TdString str) {
			std::stringstream ss;

			int i{ 1 };
			for (const auto& [id, user] : known_users)
				ss << i++ << ": " << user->username_ << " " << user->id_ << '\n';

			return make_formatted_string(ss.str());
		}}
		};

		std::array< td::td_api::object_ptr<td::td_api::formattedText>, 3> text_messages; // pseudohistory
		unsigned text_messages_index{ 0 };

	};

} //APP namespace