#include <conio.h>

using namespace std;
#include "MySQL/MySQL_Client.h"
#include "Client.hpp"
#include "Packet.hpp"

#include "Servers.hpp"

using namespace network;

#define NO_SERVERS

#ifndef NO_SERVERS
	std::shared_ptr<Server> This_Server = std::make_shared<Server>();
	std::shared_ptr<ServerFTP> This_Server_FTP = std::make_shared<ServerFTP>();
#endif

#define IP "localhost"
#define PORT 25565

// New
#define HAS_VOIP_SERVER
#ifdef HAS_VOIP_SERVER
	#include <SFML/Audio/SoundBuffer.hpp>
	#include <SFML/Audio/SoundBufferRecorder.hpp>
	#include <SFML/Audio/SoundStream.hpp>
	#include <SFML/Audio/Sound.hpp>
	#include <SFML/System/Clock.hpp>
#endif

int samplerate = 192000; // It means struct ID
enum SampleRates
{
	sr_44100 = 44100,
	sr_48000 = 48000,
	sr_96000 = 96000,
	sr_192000 = 192000,
};

#ifndef HAS_VOIP_SERVER
shared_ptr<mysql::Client> DB = make_shared<mysql::Client>();

vector<shared_ptr<Client>> Users;

	#include "File System/Level/Levels.h"
	#include "File System/File_system.h"

	shared_ptr<File_system> FS;
	#include "../File System/Project System/Project.h"

	extern std::unique_ptr<ProjectFile> Project;

	std::string Login, Pass;

//Test
nlohmann::json _Data;

bool UseRepeater = false, UseClear = false;
void ConnectFunc(string _Login, string _Pass)
{
	if (_Login.empty() || _Pass.empty()) return;

	Login = _Login;

	Users.push_back(make_shared<net::Client>("", (int)ConnectionManager::TypeProtocol::TCP, 0));
#if defined(USE_SSL)
	Users.back()->Set_Cert_Chain("keys/rootca.crt");
	Users.back()->Set_Cert_RSA_Private("keys/user.key");
#endif
	if (!Users.back()->Connect(IP, PORT, _Login, _Pass))
	{
		system("cls");
#if __has_include("logger.h")
		Logger_Error_F("User: {}, didn't connect to {}", _Login, IP);
#endif
	}
}
void GetPacketFromThread(network::Packet packet, network::Packet::Type NeededPacket)
{
	/*
	if (packet && packet.getHeader().type == network::Packet::Type::Sync_File_Sync)
	{
		auto Obj = FS->GetFile(packet.getData()["_0"].get<json::string_t>());
		if (Obj && Obj->Size > 0)
		{
			std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
			packet->CreatePacket(network::Packet::Type::Sync_File_Sync, true,
				{
					{ "_0", "01.08.16.wav" },
					{ "_1", "AD0234829205B9033196BA818F7A872B" }
				});

			Users.back()->Send(packet);

			DebugBreak();
		}
		else
		{
			std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
			packet->CreatePacket(network::Packet::Type::Sync_File_Sync, true,
				{
					{ "_0", "01.08.16.wav" },
					{ "_1", "0" }
				});

			Users.back()->Send(packet);
		}

		return;
	}
	*/
	if (packet && packet.getHeader().type & (int)network::Packet::Type::Sync_File && packet.getHeader().IsAnswer)
	{
		Users.back()->GetConnect()->getFtpClient()->Connect("127.0.0.1", Login, Pass, 21);
		auto Data = packet.getData();
		if (Data.find("_0") != Data.end() && (Data["_0"].is_boolean() && Data["_0"].get<json::boolean_t>()))
		{
			auto FName = Data["FName"].get<json::string_t>();
			std::string Path;
			if (Users.back()->GetConnect()->getFtpClient()->ReceiveFile(Data["_1"].get<json::string_t>(),
				Path = FS->getPathFromType(FS->GetTypeFileByExt(FName)) + FName))
			{
				Logger_Debug_F("File: {}. Successfully Downloaded And Placed In: {}", FName, Path);
			}
			else
			{
				Logger_Error_F("File: {}. Unsuccessful. See Logs! It Must Be In: {}", FName, Path);
			}
		}
		return;
	}
	if (packet && (packet.getHeader().type & (int)NeededPacket) && packet.getHeader().IsAnswer)
	{
		json unparsed = packet.getData();
		size_t i = 0;
		for (size_t i = 0; i < unparsed["_0"].size(); i++)
		{
#if __has_include("logger.h")
			Logger_Info_F("\tID: {} - Name: {}\n", unparsed["_1"].at(i).front().get<json::number_integer_t>(),
				unparsed["_0"].at(i).front().get<json::string_t>());
#endif
			i++;
		}
	}
}
#endif

#ifdef HAS_VOIP_SERVER
	std::shared_ptr<Client> This_Client = std::make_shared<Client>("", (int)ConnectionManager::TypeProtocol::TCP |
	(int)ConnectionManager::TypeProtocol::VOIP, 0);

	bool isPlayback = false;

	struct VOIP: public sf::SoundRecorder
	{
		VOIP(int countChannels = 2)
		{
			if (isAvailable())
				setChannelCount(countChannels);
		}
		struct Samples
		{
			Samples(sf::Int16 const* ss, std::size_t count)
			{
				samples.reserve(count);
				std::copy_n(ss, count, std::back_inserter(samples));
			}
			Samples() = default;
			std::vector<sf::Int16> samples;
		};
		class Playback : private sf::SoundRecorder, private sf::SoundStream
		{
		public: /** API **/
			// Initialise capturing input & setup output
			void start()
			{
				if (sf::SoundRecorder::start(SampleRates::sr_48000))
				{
					sf::SoundStream::initialize(sf::SoundRecorder::getChannelCount(), sf::SoundRecorder::getSampleRate());
					sf::SoundStream::play();
				}
			}
			// Stop both recording & playback
			void stop()
			{
				sf::SoundRecorder::stop();
				sf::SoundStream::stop();
			}
			bool isRunning()
			{
				return isRecording;
			}
			~Playback() { stop(); }
		protected: /** OVERRIDING SoundRecorder **/
			bool onProcessSamples(sf::Int16 const* samples, std::size_t sampleCount) override
			{
				{
					std::lock_guard<std::mutex> lock(mutex);
					data.emplace(samples, sampleCount);
				}
				cv.notify_one();
				return true; // continue capture
			}
			bool onStart() override
			{
				isRecording = true;
				return true;
			}
			void onStop() override
			{
				isRecording = false;
				cv.notify_one();
			}
		protected: /** OVERRIDING SoundStream **/
			bool onGetData(Chunk& chunk) override
			{
				// Wait until either:
				//  a) the recording was stopped
				//  b) new data is available
				std::unique_lock<std::mutex> lock(mutex);
				cv.wait(lock, [this] { return !isRecording || !data.empty(); });

				// Lock was acquired, examine which case we're into:
				if (!isRecording)
					return false; // stop playing.
				else
				{
					assert(!data.empty());

					playingSamples.samples = std::move(data.front().samples);
					data.pop();
					chunk.sampleCount = playingSamples.samples.size();
					chunk.samples = playingSamples.samples.data();
					return true;
				}
			}
			void onSeek(sf::Time) override { /* Not supported, silently does nothing. */ }
		private:
			std::atomic<bool> isRecording{ false };
			std::mutex mutex; // protects `data`
			std::condition_variable cv; // notify consumer thread of new samples
			std::queue<Samples> data; // samples come in from the recorder, and popped by the output stream
			Samples playingSamples; // used by the output stream.
		};
		class Play: public sf::SoundStream
		{
		public:
			////////////////////////////////////////////////////////////
			/// Default constructor
			///
			////////////////////////////////////////////////////////////
			Play(int SampleRate = SampleRates::sr_48000)
			{
				// Set the sound parameters
				initialize(2, SampleRate);
				start();
			}

			////////////////////////////////////////////////////////////
			/// Run the server, stream audio data from the client
			///
			////////////////////////////////////////////////////////////
			void start()
			{
				// Start playback
				play();

				// Start receiving audio data
				receiveLoop();
			}
		private:
			////////////////////////////////////////////////////////////
			/// /see SoundStream::OnGetData
			///
			////////////////////////////////////////////////////////////
			bool onGetData(sf::SoundStream::Chunk& data) override
			{
				if (!This_Client->GetConnect() || !This_Client->GetConnect()->IsConnected()) return false;
				
				// No new data has arrived since last update: wait until we get some
				while (m_offset >= m_samples.size())
					std::this_thread::sleep_for(100ms);

				m_offset = 0;
				m_tempBuffer.clear();

				// Copy samples into a local buffer to avoid synchronization problems
				// (don't forget that we run in two separate threads)
				{
					std::scoped_lock lock(m_mutex);
					m_tempBuffer.assign(m_samples.begin() + static_cast<std::vector<sf::Int64>::difference_type>(m_offset), m_samples.end());
				}

				// Fill audio data to pass to the stream
				data.samples = m_tempBuffer.data();
				data.sampleCount = m_tempBuffer.size();

				// Update the playing offset
				m_offset += data.sampleCount;
				m_samples.clear();

				return true;
			}

			////////////////////////////////////////////////////////////
			/// /see SoundStream::OnSeek
			///
			////////////////////////////////////////////////////////////
			void onSeek(sf::Time timeOffset) override
			{
				m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) * getSampleRate() * getChannelCount() / 1000;
			}

			////////////////////////////////////////////////////////////
			/// Get audio data from the client until playback is stopped
			///
			////////////////////////////////////////////////////////////
			void receiveLoop()
			{
				while (This_Client->GetConnect() && This_Client->GetConnect()->IsConnected())
				{
					std::scoped_lock lock(m_mutex);
					network::Packet packet = network::Packet();
					This_Client->GetConnect()->GetPacket(packet, (int)network::Packet::Type::VOIP);
					if (packet.getDataSize() > 0)
					{
						std::size_t sampleCount = packet.getDataSize(true) / 2 /*sizeof(sf::Int16)*/;

						// Don't forget that the other thread can access the sample array at any time
						// (so we protect any operation on it with the mutex)
						{
							std::size_t oldSize = m_samples.size();
							m_samples.resize(oldSize + sampleCount);
							std::memcpy(&(m_samples[oldSize]), static_cast<const char*>(packet.getRAWData(true)),
								sampleCount /** sizeof(sf::Int16)*/);
						}
					}
					std::this_thread::sleep_for(5ms);
				}
			}

			////////////////////////////////////////////////////////////
			// Member data
			////////////////////////////////////////////////////////////
			std::recursive_mutex  m_mutex;
			std::vector<sf::Int16> m_samples;
			std::vector<sf::Int16> m_tempBuffer;
			std::size_t m_offset = 0;
		};
		bool onProcessSamples(const sf::Int16 *samples, std::size_t sampleCount) override
		{
			if (!This_Client->GetConnect() || !This_Client->GetConnect()->IsConnected()) return false;

			dataToSend.insert(dataToSend.end(), samples, samples + (sampleCount * 2));
			if (dataToSend.size() >= 65565)
			{
				// Pack the audio samples into a network packet
				std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				packet->CreatePacket((int)network::Packet::Type::VOIP, false);
				packet->onReceive(dataToSend.data(), dataToSend.size() /*sizeof(sf::Int16)*/);

				This_Client->Send(packet);

				dataToSend.clear();
			}
			// Send the audio packet to the server
			return true;
		}

		std::vector<char> dataToSend;
	};
#endif

static int getAnswer()
{
	std::string answer;
	std::cin >> answer;
	return std::atoi(answer.c_str());
}


int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");


#ifndef NO_SERVERS
	This_Server->Start();
	This_Server_FTP->Start();

	FS = make_shared<File_system>();

	std::thread([&]
	{
		while (true)
		{
			if (Project && (!Project->GetCurrentProject().empty() && Project->ThisLevel))
			{
				Project->ThisLevel->Update();
				auto Objs = Project->ThisLevel->getChild()->GetNodes();
				for (size_t i = 0; i < Objs.size(); i++)
				{
					auto Pos = Objs.at(i)->GM->GetPositionCord();
					float vPos[3] = { Pos.x, Pos.y, Pos.z };

					auto Rot = Objs.at(i)->GM->GetRotCord();
					float vRot[3] = { Rot.x, Rot.y, Rot.z };

					auto Scl = Objs.at(i)->GM->GetScaleCord();
					float vScl[3] = { Scl.x, Scl.y, Scl.z };

					//Logger_Info_F("Model indx: '%i', "\
					//	"Model ID: '%s', "\
					//	"Model R_ID: '%s', "\
					//	"Model Pos 'X=%f, Y=%f, Z=%f', "\
					//	"Model Scl 'X=%f, Y=%f, Z=%f', "\
					//	"Model Rot 'X=%f, Y=%f, Z=%f'",
					//	i, Objs.at(i)->ID.c_str(),
					//	Objs.at(i)->RenderName.c_str(),
					//	vPos[0], vPos[1], vPos[2],
					//	vScl[0], vScl[1], vScl[2],
					//	vRot[0], vRot[1], vRot[2]);

					Objs.front()->GM->SetPositionCoords({ random_floats(0, 3), random_floats(0, 2), random_floats(1,1) });
					this_thread::sleep_for(1s);
				}
			}
		}
	}).detach();

	auto fData = FS->LoadSettingsFile();
	Project->OpenOrCreateDB();

#else
	//std::shared_ptr<VOIP> sr_network = std::make_shared<VOIP>();
	//std::shared_ptr<VOIP::Playback> sr_playback = std::make_shared<VOIP::Playback>();

	//std::thread ReceiveThread;

	//// Check that the device can capture audio
	//if (!sr_network->isAvailable())
	//{
	//	std::cout << "Sorry, audio capture is not supported by your system" << std::endl;
	//	return -1;
	//}
	//std::shared_ptr<Server> This_Server = std::make_shared<Server>(IP,
	//	(int)ConnectionManager::TypeProtocol::TCP | (int)ConnectionManager::TypeProtocol::VOIP);
#endif

	int Choice = 0;
	while (true)
	{
#ifndef HAS_VOIP_SERVER
#if __has_include("logger.h")
		Logger_Info("Choice The One:\n");
		Logger_Info("\t[0] - Login Under All Users That Are Free Now\n");
		Logger_Info("\t[1] - Login Under Needed Account\n");
		Logger_Info("\t[2] - Exit\n");

		Logger_Info(": ");
#endif
#else
		bool NeedToBreak = false;
		do
		{
#if __has_include("logger.h")
			Logger_Info("Choice The One:\n");
			Logger_Info("\t[0] - Start Client VOIP\n");
			Logger_Info("\t[1] - Start Server VOIP\n");
			Logger_Info("\t[2] - Start Playback Your Mic\n");
			Logger_Info("\t[3] - Stop Playback Your Mic\n");
			Logger_Info("\t[4] - Exit\n");

			Logger_Info(": ");
#endif
#endif
			cin >> Choice;
			switch (Choice)
			{
#ifdef HAS_VOIP_SERVER
			case 0: // Client
				while (true)
				{
					string _IP;
					system("cls");
#if __has_include("logger.h")
					Logger_Info("Enter IP:PORT Here: ");
					cin >> _IP;
#endif
					if (_IP.find(':') == std::string::npos)
					{
#if __has_include("logger.h")
						Logger_Info("Incorrect IP:PORT. Please, Try Again. Make Sure You Entered The Right IP And Port!\n");
#endif
					}
					else
					{
						string _Login, _Pass;
						system("cls");
#if __has_include("logger.h")
						Logger_Info("Enter Login Here: ");
						cin >> _Login;
#endif
#if __has_include("logger.h")
						Logger_Info("Enter Password Here: ");
						cin >> _Pass;
#endif

						if (This_Client->Connect(_IP.substr(0, _IP.find(':')),
							(USHORT)std::atoi(_IP.substr(_IP.find(':') + 1).c_str()), _Login, _Pass))
						{
							std::thread([&]
							{
								std::shared_ptr<VOIP::Play> voip_play = std::make_shared<VOIP::Play>(sr_48000);
								while (true)
								{
									// Leave some CPU time for other threads
									std::this_thread::sleep_for(std::chrono::milliseconds(100));
								}
							}).detach();
						}
						break;
					}
				}
				break;
			case 1: // Server
			{
				//This_Server->Start();

				// Not Let Go!
				NeedToBreak = true;
				break;
			}
			case 2: // Playback
				isPlayback = true;
				//sr_playback->start();
				break;
			case 3: // Playback
				isPlayback = false;
				//if (sr_playback->isRunning())
				//	sr_playback->stop();
				break;
#else
			case 0:
			{
				auto AllUsers = DB->SelectValues("Local", { "*" }, { " WHERE _2 = 0" });
				for (size_t i = 0; i < AllUsers["_N"].size(); i++)
				{
					ConnectFunc(AllUsers.at(i).get<json::string_t>(), AllUsers["_1"].at(i).get<json::string_t>());
				}
				break;
			}
			case 1:
			{
				string _Login, _Pass;
				system("cls");
#if __has_include("logger.h")
				Logger_Info("Enter Login Here: ");
				cin >> _Login;
#endif
#if __has_include("logger.h")
				Logger_Info("Enter Password Here: ");
				cin >> _Pass;
#endif

				ConnectFunc(_Login, _Pass);

				break;
			}
#endif
			case 4:
			{
				return 0;
			}
			default:
			{
				system("cls");
#if __has_include("logger.h")
				Logger_Error("Unrecognized Choice. Try Another One!\n");
#endif
				continue;
			}
			}

#ifdef HAS_VOIP_SERVER
		} while (isPlayback || NeedToBreak);
#endif
		Sleep(1000);
		Choice = 0;

#ifndef HAS_VOIP_SERVER
		string Text;
#endif
#ifndef HAS_VOIP_SERVER
		std::thread([&]
		{
			while (!Users.empty())
			{
				Sleep(100);
				network::Packet packet;

				if (Users.size() == 1)
				{
					if (!UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
						packet.clear();

						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
						GetPacketFromThread(packet, network::Packet::Type::Sync_File);
						packet.clear();
					}
					while (UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
						packet.clear();

						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
						GetPacketFromThread(packet, network::Packet::Type::Sync_File);
						packet.clear();
					}
				}
				else if (Users.size() > 1)
				{
					if (!UseRepeater)
					{
						for (auto CurrentUser : Users)
						{
							if (!CurrentUser->GetConnect()) continue;
							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
							GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
							packet.clear();

							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
							GetPacketFromThread(packet, network::Packet::Type::Sync_File);
							packet.clear();
						}
					}
					while (UseRepeater)
					{
						for (auto CurrentUser : Users)
						{
							if (!CurrentUser->GetConnect()) continue;
							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
							GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
							packet.clear();

							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
							GetPacketFromThread(packet, network::Packet::Type::Sync_File);
							packet.clear();
						}
					}
				}

			}
		}).detach();
#endif
		while (true)
		{
			Sleep(1000);

#ifdef HAS_VOIP_SERVER
			if (Choice == 2 || !(This_Client->IsRunning() || This_Client->GetConnect() && This_Client->GetConnect()->IsConnected()))
				break;
#if __has_include("logger.h")
			Logger_Info("Choice The One:\n");
			Logger_Info("\t[0] - Start Capture Sound (Only For Client)\n");
			Logger_Info("\t[1] - Stop Capture Sound (Only For Client)\n");

			Logger_Info("\t[2] - Back To Previous Menu\n");

			Logger_Info(": ");
#endif
#else
#if __has_include("logger.h")
			Logger_Info("Choice The One:\n");
			Logger_Info("\t[0] - Get Ping (ECHO)\n");
			Logger_Info("\t[1] - Send Chat Message Packet\n");
			Logger_Info("\t[2] - Get File\n");
			Logger_Info("\t[3] - Send \"Sound Play\" Packet (By Default It's \"01.08.16.wav\")\n");

			Logger_Info("\t[4] - Get List Online Users\n");

			Logger_Info("\t[5] - Back To Previous Menu\n");

			Logger_Info_F("\t[6] - Use Repeater Any Packets ({} - is now)\n", UseRepeater ? "ON" : "OFF");

			Logger_Info_F("\t[7] - Use Clear Screen After Command ({} - is now)\n", UseClear ? "ON" : "OFF");

			Logger_Info(": ");
#endif
#endif
			while (true)
			{
				std::future<int> future = std::async(getAnswer);
				if (future.wait_for(std::chrono::hours(100000)) == std::future_status::ready)
				{
					Choice = future.get();
					break;
				}
				std::this_thread::sleep_for(100ms);
			}
			switch (Choice)
			{
#ifdef HAS_VOIP_SERVER
			case 0:
			{
				bool need_to_exit = false;
				while (!need_to_exit)
				{
#if __has_include("logger.h")
					Logger_Info("Choice The One:\n");
					Logger_Info("\t[1] - Sample Rate 44100 (CD)\n");
					Logger_Info("\t[2] - Sample Rate 48000 (DVD)\n");
					Logger_Info("\t[3] - Sample Rate 96000 (Studio)\n");
					Logger_Info("\t[4] - Sample Rate 192000 (Studio)\n");

					Logger_Info(": ");
#endif

					while (true)
					{
						std::future<int> future = std::async(getAnswer);
						if (future.wait_for(std::chrono::hours(100000)) == std::future_status::ready)
						{
							samplerate = future.get();
							break;
						}
						std::this_thread::sleep_for(100ms);
					}
					switch (samplerate)
					{
					case 1:
						samplerate = sr_44100;
						break;
					case 2:
						samplerate = sr_48000;
						break;
					case 3:
						samplerate = sr_96000;
						break;
					case 4:
						samplerate = sr_192000;
						break;
					default:
					{
						system("cls");
#if __has_include("logger.h")
						Logger_Error("Unrecognized Choice. Try Another One!\n");
#endif
						continue;
					}
					}
					need_to_exit = true;
				}

				// Start capturing audio data
				//if (!sr_network->start(samplerate))
				{
					std::cerr << "Failed to start recorder" << std::endl;
					return -1;
				}

				std::cout << "Recording...\n";
				std::cin.ignore(10000, '\n');
				break;
			}
			case 1:
			{
				//sr_network->stop();
				break;
			}
			case 2:
			{
				break;
			}
#else
			case 0:
			{
				std::thread t = std::thread([&]
				{
					std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
					packet->CreatePacket((int)network::Packet::Type::Ping, false);

					if (Users.size() == 1)
					{
						if (UseRepeater)
						{
							while (UseRepeater)
							{
								if (Users.front()->GetConnect())
									Users.front()->GetConnect()->Send(packet);
							}
						}
						else
						{
							if (Users.front()->GetConnect())
								Users.front()->GetConnect()->Send(packet);
						}
					}
					else if (Users.size() > 1)
					{
						if (UseRepeater)
						{
							while (UseRepeater)
							{
								for (auto CurrentUser : Users)
								{
									if (!CurrentUser->GetConnect()) continue;
									CurrentUser->GetConnect()->Send(packet);
								}
							}
						}
						else
						{
							for (auto CurrentUser : Users)
							{
								if (!CurrentUser->GetConnect()) continue;
								CurrentUser->GetConnect()->Send(packet);
							}
						}
					}
				});
				if (UseRepeater)
					t.detach();
				else
					t.join();

				break;
			}
			case 1:
			{
				Text.clear();
				system("cls");
#if __has_include("logger.h")
				Logger_Info("Enter Message Here: ");
#endif
				cin >> Text;

				std::thread t = std::thread([&]
				{
					std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
					packet->CreatePacket((int)network::Packet::Type::Chat, false, { { "_0", Text + "\n" } });

					if (Users.size() == 1)
					{
						if (UseRepeater)
						{
							while (UseRepeater)
							{
								if (Users.front()->GetConnect())
									Users.front()->GetConnect()->Send(packet);
							}
						}
						else
						{
							if (Users.front()->GetConnect())
								Users.front()->GetConnect()->Send(packet);
						}
					}
					else if (Users.size() > 1)
					{
						if (UseRepeater)
						{
							while (UseRepeater)
							{
								for (auto CurrentUser : Users)
								{
									if (!CurrentUser->GetConnect()) continue;
									CurrentUser->GetConnect()->Send(packet);
								}
							}
						}
						else
						{
							for (auto CurrentUser : Users)
							{
								if (!CurrentUser->GetConnect()) continue;
								CurrentUser->GetConnect()->Send(packet);
							}
						}
					}
				});
				if (UseRepeater)
					t.detach();
				else
					t.join();

				break;
			}
			case 2:
			{
				//std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				//packet->CreatePacket(network::Packet::Type::Sync_NewNode, true,
				//{
				//	{ "FName", "01.08.16.wav" },
				//	{ "id", "01.08.16" }
				//});
				std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				packet->CreatePacket((int)network::Packet::Type::Sync_File, false,
					{ { "FName", "01.08.16.wav" } });

				Users.back()->Send(packet);
				break;
			}
			case 3:
			{
				Text.clear();
				system("cls");
#if __has_include("logger.h")
				Logger_Info("Enter ID User Here (or '-1' To All Users That Are Online): ");
#endif
				cin >> Text;

				std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				packet->CreatePacket((int)network::Packet::Type::Chat, false, _Data);
				if (Users.front()->GetConnect())
					Users.front()->GetConnect()->Send(packet);

				break;
				// Doesn't Work Because DB Was Overwritten By Changing Design Of Networking
				if (Text == "-1")
				{
					auto AllUsersID = DB->SelectValues("Local", { "_N" }, { " WHERE _N = 1" });
					vector<std::shared_ptr<network::Packet>> packet;

					for (auto ID : AllUsersID)
					{
						packet.push_back(std::make_shared<network::Packet>());
						packet.back()->CreatePacket((int)network::Packet::Type::PlaySound, false,
							{
								{ "_0", ID.back().get<json::number_integer_t>() },
								{ "_1", 0.46f },
								{ "_2", "01.08.16.wav" }
							});
					}
					for (auto ThisPacket : packet)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->Send(ThisPacket);
					}
				}
				else
				{
					std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
					//packet->CreatePacket(network::Packet::Type::PlaySound, false,
					//	{
					//		{ "_0", atoi(Text.c_str()) },
					//		{ "_1", 0.016f },
					//		{ "_2", "01.08.16.wav" }
					//	});
					packet->CreatePacket((int)network::Packet::Type::Chat, false, _Data);
					if (Users.front()->GetConnect())
						Users.front()->GetConnect()->Send(packet);
				}
				break;
			}
			case 4:
			{
				std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				packet->CreatePacket((int)network::Packet::Type::GetListUsersOnline, false);

				if (Users.size() == 1)
				{
					if (!Users.front()->GetConnect()) continue;
					Users.front()->GetConnect()->Send(packet);
				}
				else if (Users.size() > 1)
				{
					for (auto CurrentUser : Users)
					{
						if (!CurrentUser->GetConnect()) continue;
						CurrentUser->GetConnect()->Send(packet);
					}
				}

				Sleep(1000);
				break;
			}
			case 5:
			{
				break;
			}
			case 6:
			{
				UseRepeater = !UseRepeater;
				break;
			}
			case 7:
			{
				UseClear = !UseClear;
				break;
			}
			}
			if (Choice == 5)
				break;

			if (UseClear)
			{
				Sleep(1000);
				system("cls");
			}
#endif
			}
		}
	}
#ifndef NO_SERVERS
This_Server->StopSystem();
This_Server_FTP->StopSystem();
#endif

return 0;
}
