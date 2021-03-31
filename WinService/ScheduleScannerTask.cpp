#include "ScheduleScannerTask.h"
#include <chrono>
#include <ctime>
using namespace std::chrono;
std::condition_variable cv;

void ScheduleScannerTask::cancel()
{
	toCancel = true;
	cv.notify_one();
}
std::mutex cv_m,cv_m2;
void ScheduleScannerTask::schedule(int64_t scTime,std::filesystem::path path)
{
	duration<int64_t> dur(scTime);
	scheduledTime = dur;
	currTime = duration_cast<seconds>(system_clock::now().time_since_epoch());
	int minute = std::chrono::duration_cast<std::chrono::minutes>(currTime).count();
	int minuteScheduled = std::chrono::duration_cast<std::chrono::minutes>(scheduledTime).count();
	if (minuteScheduled < minute)
	{
		_ts = TaskStatus::Failed;
		return;
	}
	_ts = TaskStatus::Scheduled;
	std::chrono::seconds delta = scheduledTime - currTime;
//	std::this_thread::sleep_for(delta);
	std::unique_lock<std::mutex> lk(cv_m);
	cv.wait_for(lk, delta, [&]() {return toCancel; });
	if (toCancel)
	{
		toCancel = false;
		_ts = TaskStatus::Complete;
		return;
	}
	scan(path);
}
