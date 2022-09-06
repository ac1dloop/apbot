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

		//tdlib settings
		constexpr auto tg_api_id = 7809655;
		constexpr std::string_view tg_api_hash = "1de39b873d05798b88ce9e49e5f6dfba";
		constexpr auto db_dir = "tdlib";
		constexpr auto files_dir = "dl";
		constexpr auto system_lang = "en";
		constexpr auto app_ver = "1.0";
		constexpr auto device_model = "Raspberry pi";
		constexpr auto system_version = "Linux"; 

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
		using CommandHandler = std::function<std::optional<std::string>(std::string)>;

		using ResponseHandler = std::function<void(Object)>;
		using ClientId = td::ClientManager::ClientId;
		using MessageSender = td::td_api::object_ptr <td::td_api::MessageSender>;
		using BotId = td::td_api::int53;
		using ChatId = td::td_api::int53;
		using UserId = td::td_api::int53;

		explicit Bot(int log_verbosity = 0) {

			td::ClientManager::execute(td::td_api::make_object<td::td_api::setLogVerbosityLevel>(log_verbosity));

			client_id = client_manager->create_client_id();

			make_request(td::td_api::make_object<td::td_api::getOption>("version"), [this](Object obj) {
					auto optionValue = td::td_api::move_object_as<td::td_api::optionValueString>(obj);
					std::cout << "tdlib version " << optionValue->value_ << '\n';
			});
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
				parameters->database_directory_ = CONST::db_dir;
				parameters->files_directory_ = CONST::files_dir; //downloads folder
				parameters->use_file_database_ = true;
				parameters->use_chat_info_database_ = true;
				parameters->use_message_database_ = true;
				parameters->api_id_ = CONST::tg_api_id;
				parameters->api_hash_ = CONST::tg_api_hash;
				parameters->system_language_code_ = CONST::system_lang;
				parameters->device_model_ = CONST::device_model;
				parameters->application_version_ = CONST::app_ver;
				parameters->system_version_ = CONST::system_version;
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

			if (sender->get_id() != td::td_api::messageSenderUser::ID) {
				//message from chat
				//ignoring...
				return;
			}

			//get id
			auto sender_user = td::td_api::move_object_as<td::td_api::messageSenderUser>(sender);

			auto known_user = known_users.find(sender_user->user_id_);

			if (known_user == known_users.end()) {
				//cannot process message
				return;
			}

			//test if pidor
			auto pidor = known_pidors.find(sender_user->user_id_);

			if (pidor != known_pidors.end()) {
				//process pidor

				auto send_message = td::td_api::make_object<td::td_api::sendMessage>();

				send_message->reply_to_message_id_ = message->id_;
				send_message->chat_id_ = message->chat_id_;

				//message settings
				//auto message_settings = td::td_api::make_object<td::td_api::messageSendOptions>();
				//message_settings->disable_notification_ = true;
				//message_settings->from_background_ = true;
				//message_settings->protect_content_ = true;
				//send_message->options_ = std::move(message_settings);

				//actual content of message
				auto message_content = td::td_api::make_object<td::td_api::inputMessageText>();
				message_content->text_ = td::td_api::make_object<td::td_api::formattedText>();
				message_content->text_->text_ = pidor->second;

				send_message->input_message_content_ = std::move(message_content);

				make_request(std::move(send_message));

				return;
			}

			//now we care about content
			auto message_content = std::move(message->content_);

			//can process only text
			if (message_content->get_id() != td::td_api::messageText::ID) {
				return;
			}

			auto text_content = td::td_api::move_object_as<td::td_api::messageText>(message_content);
			auto form_text = std::move(text_content->text_);

			std::string str = form_text->text_;

			//test if command
			if (str.size() != 0 && str.at(0) == '/') {

				std::optional<std::string> cmd_result{ "unauthorized" };

				if (known_user->second->is_contact_)
					cmd_result = process_command(str);

				if (!cmd_result.has_value())
					return;

				auto send_message = td::td_api::make_object<td::td_api::sendMessage>();

				send_message->reply_to_message_id_ = message->id_;
				send_message->chat_id_ = message->chat_id_;

				//actual content of message
				auto message_content = td::td_api::make_object<td::td_api::inputMessageText>();
				message_content->text_ = td::td_api::make_object<td::td_api::formattedText>();
				message_content->text_->text_ = cmd_result.value();

				send_message->input_message_content_ = std::move(message_content);

				make_request(std::move(send_message));

				return;
			}

			//else append to pseudohistory
			text_messages[text_messages_index % text_messages.size()] = std::move(form_text);
			text_messages_index++;
		}

		std::optional<std::string> process_command(std::string str) {
			//ignore starting slash
			auto fsp = str.find(' ');
			auto command = str.substr(1, fsp - 1);

			auto cmd_iter = command_handlers.find(command);

			if (cmd_iter == command_handlers.end())
				return {};

			return cmd_iter->second(str.substr(fsp + 1, std::string::npos));
		}

		void make_request(Request req, ResponseHandler handler) {
			client_manager->send(client_id, ++request_id, std::move(req));
			handlers.emplace(request_id, handler);
		}

		void make_request(Request req) {
			client_manager->send(client_id, ++request_id, std::move(req));
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
		std::unordered_map<UserId, std::string> known_pidors{
			{CONST::pidorbot_id, CONST::pidorbot_message}
		};

		std::unordered_map<std::string, CommandHandler> command_handlers
		{
		{"help", [](std::string) {
				std::stringstream ss;

				ss << "help - get help\n";
				ss << "setloglevel - flood console\n";
				ss << "addpidor_id [id] [text] - add pidor to hatelist\n";
				ss << "addpidor_name [username] [text]\n";
				ss << "delpidor_id\n";
				ss << "delpidor_name\n";
				ss << "listpidors\n";

				return ss.str();
		}},
		{"setloglevel", [this](std::string str) {
				int lvl;

				try {
					lvl = std::stoi(str);
				}
				catch (...) {
					return "failed";
				}

				td::ClientManager::execute(td::td_api::make_object<td::td_api::setLogVerbosityLevel>(lvl));

				return "success";
		}},
		{"addpidor_id", [this](std::string str) {
			auto fsp = str.find(' ');

			UserId id;

			try {
				id = std::stoll(str.substr(0, fsp));
			}
			catch (...) {
				return "failed";
			}

			known_pidors[id] = str.substr(fsp + 1, std::string::npos);

			return "success";
		}},
		{"addpidor_name", [this](std::string str) {
			auto fsp = str.find(' ');

			auto name = str.substr(0, fsp);
			UserId uid = 0;

			for (const auto& [id, user] : known_users) {
				if (user->username_ == name) {
					uid = id;
					break;
				}
			}

			if (uid == 0)
				return "failed";

			known_pidors[uid] = str.substr(fsp + 1, std::string::npos);

			return "success";
		}},
		{"delpidor_id", [this](std::string str) {
			auto fsp = str.find(' ');

			UserId id;

			try {
				id = std::stoll(str.substr(0, fsp));
			}
			catch (...) {
				return "failed";
			}

			auto pidor_it = known_pidors.find(id);

			if (pidor_it != known_pidors.end())
				known_pidors.erase(pidor_it);

			return "success"; 
		}},
		{"delpidor_name", [this](std::string str) {
			auto fsp = str.find(' ');

			auto name = str.substr(0, fsp);
			UserId uid = 0;

			for (const auto& [id, user] : known_users) {
				if (user->username_ == name) {
					uid = id;
					break;
				}
			}

			if (uid == 0)
				return "failed";

			auto pidor_it = known_pidors.find(uid);

			if (pidor_it != known_pidors.end())
				known_pidors.erase(pidor_it);

			return "success";
		}},
			
		{"listpidors", [this](std::string str) {
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

					ss << " [" << text << "]\n";
				}

				return ss.str();
		}},
		};

		std::array< td::td_api::object_ptr<td::td_api::formattedText>, 3> text_messages; // pseudohistory
		unsigned text_messages_index{ 0 };

	};

} //APP namespace