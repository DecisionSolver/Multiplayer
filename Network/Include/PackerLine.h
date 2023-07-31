#pragma once
class PacketLine
{
public:
	enum ReturnType
	{
		Type = 1, Status
	};
private:
	std::mutex m_PacketWaiter;
	std::atomic_bool WasPacket = false;
	std::atomic_bool wasActive = false;
	std::atomic_bool NeedToBreak = false;
	std::chrono::time_point<std::chrono::steady_clock> Cur, Last;

	// Packet Type To Check
	//		type, need to break all chain
	std::map<int, bool> TypeToCheck; // ref: network::Packet::Type

	// Packet Status (Only Works With Type)
	//		type, need to break all chain
	std::map<int, bool> StatusToCheck; // ref: network::Packet::Status

	std::pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>> CauseBreak;
public:
	//std::condition_variable cv_PacketWaiter;

	void SetTypeCauseBreak(const int &Type)
	{
		//							.first		.second			.first			.second
		CauseBreak = std::make_pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>(ReturnType::Type,
			{ (*TypeToCheck.find(Type)), { -1, false } });
	}
	void SetStatusCauseBreak(const int &Type, const int &Status)
	{
		//							.first		.second			.first			.second
		CauseBreak = std::make_pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>(ReturnType::Status,
			{ (*TypeToCheck.find(Type)), (*StatusToCheck.find(Status)) });
	}

	// Set Type To Check In Needed Packet When It Cames
	void SetType(const int &Type, bool need2break = false)
	{
		TypeToCheck.insert({ Type, need2break });
	}
	// Sets When Need To Unblock "Check" Function
	void NotifyOne()
	{
		NeedToBreak.store(true);
	}

	// Sets When Packet Has Come
	void SetWasPacket()
	{
		WasPacket.store(true);
	}
	// Set Status To Check In Needed Packet When It Cames (Only Works With Type!!!)
	void SetStatus(const int &Status, const int &Type, bool need2break = false)
	{
		TypeToCheck.insert({ Type, need2break });
		StatusToCheck.insert({ Status, need2break });
	}

	std::map<int, bool> &GetStatus()
	{
		return StatusToCheck;
	}
	std::map<int, bool> &GetType()
	{
		return TypeToCheck;
	}
	bool WasActive()
	{
		return wasActive.load();
	}
	//			//was break								// type,				// status
	std::pair<bool, std::pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>>
		Check(const std::chrono::seconds &TimeOut = 60s);
};
