#include "Service.h"

int main(int argc,TCHAR* argv[])
{
	Service* svc = new Service;
	Service::setServiceInstance(svc);
	Service::ServiceProcess(argc,argv);
	delete svc;
	return 0;
}