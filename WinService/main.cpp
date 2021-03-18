#include "Service.h"
#include <tchar.h>
#include <locale.h>
int _tmain(int argc,TCHAR* argv[])
{
	Service* svc = new Service;
	Service::setServiceInstance(svc);
	Service::ServiceProcess(argc,argv);
	delete svc;
	return 0;
}