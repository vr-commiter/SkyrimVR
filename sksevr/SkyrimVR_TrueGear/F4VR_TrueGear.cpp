#include "F4VR_TrueGear.h"

#include <F4VR_TrueGear/json.hpp>
//#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <F4VR_TrueGear/Utility.hpp>
#include <random>


//时间
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

#include "configor/json.hpp"
#include "easywsclient.hpp"
#include "concurrentqueue.h"
#include "base64.h"
#include "cmd.h"
#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#include <windows.h>
#endif


typedef int (WINAPI* MY_FUNC)(void);
MY_FUNC func;

using json = nlohmann::json;
using namespace configor;
using namespace moodycamel;

namespace F4VR_TrueGear {
	using easywsclient::WebSocket;
	std::string appId = "611660";
	std::string apiKey = "Fallout4 VR";
	bool systemInitialized = false;
	std::unique_ptr<easywsclient::WebSocket> ws;
	std::map<std::string, EffectObject> _effectSeek; //effect seek cache
	std::mutex pollingMtx; //mutex for _registered variable
	std::unique_ptr<ConcurrentQueue<std::string>> send_req_queue;

	void setupWS()
	{
		using easywsclient::WebSocket;
#ifdef _WIN32
		INT rc;
		WSADATA wsaData;

		rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (rc) {
			printf("WSAStartup Failed.\n");
			return;
		}
#endif

		ws = std::unique_ptr<WebSocket>(WebSocket::from_url("ws://127.0.0.1:18233/v1/tact/"));
		send_req_queue = std::unique_ptr<ConcurrentQueue<std::string>>(new ConcurrentQueue<std::string>());
		if (!ws)
		{
			return;
		}

#ifdef _WIN32
		Sleep(1);
#endif
		if (ws->getReadyState() == WebSocket::CONNECTING)
		{
		}
		_MESSAGE("is connected");
	}
	static bool quit_;
	static int cur;
	static std::string Generate_ReqId()
	{
		return std::to_string(cur++);
	}

	bool connectionCheck()
	{
		if (!ws)
		{
			return false;
		}

		WebSocket::readyStateValues isClosed = ws->getReadyState();
		if (isClosed == WebSocket::CLOSED)
		{
			ws.reset(nullptr);
			return false;
		}

		return true;
	}

	void rece_message() {
		ws->poll();
		ws->dispatch([](const std::string& message) {
			nlohmann::json j = nlohmann::json::parse(message);
			CmdResponse res;
			CmdResponse_from_json(j, res);
			if (res.Method == "seek_by_uuid") {
				std::string body = res.Result;
				std::string decoded_str = base64_decode(body);
				nlohmann::json v = nlohmann::json::parse(decoded_str);
				EffectObject p = EffectObject(v);
				_effectSeek[p.uuid] = p;
			}
			if (res.Method == "register_app") {
				for (auto& p : _effectSeek) {
					SendSeekEffectByUUid(p.first);
				}
			}
			});
	}

	void WsLoop()
	{
		int count = 0;
		while (!quit_)
		{

			if (!connectionCheck())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			if (count > 20)
			{
				rece_message();
				count = 0;
			}
			count++;
			std::string request;
			while (send_req_queue->try_dequeue(request))
			{
				ws->send(request);
			}

			ws->poll();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	void send1(std::string request)
	{
		send_req_queue->enqueue(request);
	}

	void SendRegist() {
		std::string content = appId + ";" + apiKey;
		std::string encoded = base64_encode(content);

		_MESSAGE("SendRegist");
		nlohmann::json json1;
		CmdRequest_to_json(json1, CmdRequest{ "register_app", encoded, Generate_ReqId() });
		std::string str = json1.dump();
		send1(str);
	}

	void SendPlay(std::string uuid) {
		std::string content = appId + ";" + uuid;
		std::string encoded = base64_encode(content);

		nlohmann::json json1;
		CmdRequest_to_json(json1, CmdRequest{ "play_effect_by_uuid", encoded, Generate_ReqId() });
		std::string str = json1.dump();
		send1(str);
	}

	void SendPlayEffectByContent(std::string content) {
		std::string encoded = base64_encode(content);
		nlohmann::json json1;
		CmdRequest_to_json(json1, CmdRequest{ "play_effect_by_content", encoded , Generate_ReqId() });
		std::string str = json1.dump();
		send1(str);
	}

	void SendSeekEffectByUUid(std::string uuid) {
		_MESSAGE("SendSeekEffectByUUid");
		_MESSAGE(uuid.c_str());
		std::string content = appId + ";" + uuid;
		std::string encoded = base64_encode(content);
		nlohmann::json json1;
		CmdRequest_to_json(json1, CmdRequest{ "seek_by_uuid", encoded , Generate_ReqId() });
		std::string str = json1.dump();
		send1(str);
	}

	void PreSeekEffect(std::string uuid)
	{
		_effectSeek[uuid] = EffectObject();
	}

	EffectObject FindEffectByUuid(std::string uuid)
	{
		return _effectSeek[uuid];
	}

	void CreateSystem()
	{
		if (!systemInitialized)
		{
			//std::thread t100();
			//t100.detach();
			setupWS();
			PreSeekEffect("PlayerBulletDamage");
			PreSeekEffect("BodyDamage");
			PreSeekEffect("DefaultDamage");
			SendRegist();
		}

		systemInitialized = true;
	}

	void Play(std::string event)
	{
		// 获取当前时间
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		struct tm localTime;
		localtime_s(&localTime, &now_c);
		std::ostringstream ss;
		ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
		ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
		std::string current_time_with_milliseconds = ss.str();

		const char* message = event.c_str();
		_MESSAGE("[%s]	event :%s", current_time_with_milliseconds.c_str(), message);

		//发送消息到Player
		SendPlay(event);
	}

	void PlayAngle(std::string event, float angle, float vertial)
	{
		// 获取当前时间
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		struct tm localTime;
		localtime_s(&localTime, &now_c);
		std::ostringstream ss;
		ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
		ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
		std::string current_time_with_milliseconds = ss.str();

		const char* message = event.c_str();
		_MESSAGE("[%s]	event :%s , angle :%f , vertial :%f", current_time_with_milliseconds.c_str(), message, angle, vertial);


		try {

			EffectObject rootObject = FindEffectByUuid(event);

			int verCount = vertial > 0.1f ? -4 : vertial < 0 ? 8 : 0;
			float tmpAngle = (angle - 22.5f) > 0 ? angle - 22.5f : 360 - angle;
			int horCount = (int)(tmpAngle / 45) + 1;

			for (auto& track : rootObject.trackList) {
				if (track.action_type == Shake)
				{
					auto& indices = track.index;
					for (auto& index : track.index) {
						int tmpIndex = index;
						if (verCount != 0)
						{
							tmpIndex += verCount;
						}
						if (horCount < 8)
						{
							if (tmpIndex < 50)
							{
								int remainder = tmpIndex % 4;
								if (horCount <= remainder)
								{
									tmpIndex = tmpIndex - horCount;
								}
								else if (horCount <= (remainder + 4))
								{
									auto num1 = horCount - remainder;
									tmpIndex = tmpIndex - remainder + 99 + num1;
								}
								else
								{
									tmpIndex = tmpIndex + 2;
								}
							}
							else
							{
								int remainder = 3 - (tmpIndex % 4);
								if (horCount <= remainder)
								{
									tmpIndex = tmpIndex + horCount;
								}
								else if (horCount <= (remainder + 4))
								{
									auto num1 = horCount - remainder;
									tmpIndex = tmpIndex + remainder - 99 - num1;
								}
								else
								{
									tmpIndex = tmpIndex - 2;
								}
							}
						}
						index = tmpIndex;
					}
					indices.erase(std::remove_if(indices.begin(), indices.end(), [](int index) {
						return index < 0 || (index > 19 && index < 100) || index > 119;
						}), indices.end());
				}
				else if (track.action_type == Electrical)
				{
					auto& indices = track.index;
					for (auto& index : track.index) {
						int tmpIndex = index;
						if (horCount <= 4)
						{
							index = 0;
						}
						else
						{
							index = 100;
						}
					}
					if (horCount == 1 || horCount == 8 || horCount == 4 || horCount == 5)
					{
						track.index = { 0, 100 };
					}
				}
			}
			std::string j = rootObject.to_json().dump();
			//PlayNoRegister
			SendPlayEffectByContent(j);
		}
		catch (const std::exception& e) {
			//Play
			SendPlay(event);
		}


	}

	void PlayLate(std::string event, int time, int count)
	{
		for (int i = 0; i < count; i++)
		{
			Sleep(time);
			// 获取当前时间
			auto now = std::chrono::system_clock::now();
			std::time_t now_c = std::chrono::system_clock::to_time_t(now);
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
			struct tm localTime;
			localtime_s(&localTime, &now_c);
			std::ostringstream ss;
			ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
			ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
			std::string current_time_with_milliseconds = ss.str();

			const char* message = event.c_str();
			_MESSAGE("[%s]	event :%s , time :%d , count :%d", current_time_with_milliseconds.c_str(), message, time, count);

			//发送消息到Player
			SendPlay(event);

		}
	}

	void PlayRandom(const std::string& event, const std::vector<std::vector<int>>& eletricals, const std::vector<int>& randomCounts) {
		try {

			EffectObject rootObject = FindEffectByUuid(event);

			int randomSum = std::accumulate(randomCounts.begin(), randomCounts.end(), 0);
			auto tmpEletricals = eletricals;
			auto tmpRandomCounts = randomCounts;

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distrib(0, 1);

			std::default_random_engine rand;
			for (auto& track : rootObject.trackList) {
				if (track.action_type == Shake)
				{
					auto& indices = track.index;
					while (indices.size() < randomSum) {
						indices.push_back(0);
					}

					int j = 0;
					for (size_t i = 0; i < indices.size(); i++) {
						if (tmpRandomCounts[j] > 1) {
							int randomCol = rand() % tmpEletricals[j].size();
							while (tmpEletricals[j][randomCol] == -1) {
								randomCol = rand() % tmpEletricals[j].size();
							}
							indices[i] = tmpEletricals[j][randomCol];
							tmpEletricals[j][randomCol] = -1;
							tmpRandomCounts[j]--;
						}
						else {
							int randomCol = rand() % tmpEletricals[j].size();
							while (tmpEletricals[j][randomCol] == -1) {
								randomCol = rand() % tmpEletricals[j].size();
							}
							indices[i] = tmpEletricals[j][randomCol];
							tmpEletricals[j][randomCol] = -1;
							tmpRandomCounts[j]--;
							j++;
						}
					}
				}
				else if (track.action_type == Electrical)
				{
					int random_number = distrib(gen);
					for (auto& index : track.index) {
						if (random_number == 0)
						{
							index = 0;
						}
						else
						{
							index = 100;
						}
					}

				}

			}
			std::string j = rootObject.to_json().dump();
			SendPlayEffectByContent(j);
		}
		catch (const std::exception& e) {
			SendPlay(event);
		}
	}



}