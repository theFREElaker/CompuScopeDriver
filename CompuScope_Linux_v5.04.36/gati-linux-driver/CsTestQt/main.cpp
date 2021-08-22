#include "cstestqt.h"
//#include <QtWidgets/QApplication>
#include <QWidget>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	CsTestQt* w;

	int result = 0;

	bool bSsm = true;
    bool bDebug = false;
    bool bAdvance = false;

	for(int i=0;i<argc;i++)
	{
        if((strcmp(argv[i], "/disp")==0) || (strcmp(argv[i], "/DISP")==0) || (strcmp(argv[i], "/Disp")==0))
		{
			bSsm = false;
		}

        if ((strcmp(argv[i], "/debug") == 0) || (strcmp(argv[i], "/DEBUG") == 0) || (strcmp(argv[i], "/Debug") == 0))
		{
			bDebug = true;
		}

        if ((strcmp(argv[i], "/advance") == 0) || (strcmp(argv[i], "/ADVANCE") == 0)  || (strcmp(argv[i], "/Advance") == 0))
        {
            bAdvance = true;
        }
	}

	try
	{
        w = new CsTestQt(bSsm, bDebug, bAdvance);
	}
	catch(int error)
	{
		// TBD with the error cathed
		error = error;
		return 0;
	}

	w->show();

	try
	{
		result = a.exec();
	}
	catch(int error)
	{
		// TBD with the error cathed
		error = error;

		if(w->isVisible()) w->close();
		delete w;
		return result;
	}

	if(w->isVisible()) w->close();
	delete w;
	return result;
}
