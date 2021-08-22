#include "qaboutdialog.h"

#ifdef Q_OS_WIN
    #pragma comment(lib, "version.lib")
#endif

QAboutDialog::QAboutDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

	//TODO: Fill the label values
	#ifdef Q_OS_WIN
		ui.lblSsmPath->setText(getDllPath("CsSsm"));
		ui.lblDispPath->setText(getDllPath("CsDisp"));

		ui.lblSsmVersion->setText(getDllVersion(ui.lblSsmPath->text()));
		ui.lblDispVersion->setText(getDllVersion(ui.lblDispPath->text()));
	#else
		ui.lblSsmPath->setText("NOT IMPLEMENTED");
		ui.lblDispPath->setText("NOT IMPLEMENTED");

		ui.lblSsmVersion->setText("NOT IMPLEMENTED");
		ui.lblDispVersion->setText("NOT IMPLEMENTED");
	#endif

		ui.lblVersion->setText("1.0.1 (0001)");
}

QAboutDialog::~QAboutDialog()
{

}

#ifdef Q_OS_WIN

QString QAboutDialog::getDllPath(const QString& strDllName)
{
	wchar_t strName[MAX_PATH];
	strDllName.toWCharArray(strName);
	strName[strDllName.size()] = '\0';

	HMODULE hModule = GetModuleHandle(strName);

	wchar_t strPath[MAX_PATH];
	GetModuleFileName(hModule, strPath, 256);
	return QString::fromWCharArray(strPath);
}

QString QAboutDialog::getDllVersion(const QString& strDllFileName)
{
	wchar_t strFileName[MAX_PATH];
	strDllFileName.toWCharArray(strFileName);
	strFileName[strDllFileName.length()] = '\0';

	uInt32 u32Dummy;
	uInt32 u32Size = ::GetFileVersionInfoSize(strFileName, &u32Dummy);

	if (u32Size <= 0)
		return "ERROR";

	LPVOID pVersion = ::VirtualAlloc(NULL, u32Size, MEM_COMMIT, PAGE_READWRITE);

	if (pVersion == 0)
		return "ERROR";

	QString strVersion = "Error";
	if (::GetFileVersionInfo(strFileName, u32Dummy, u32Size, pVersion))
	{
		VS_FIXEDFILEINFO*	pVersionInfo; 
		UINT				u32VersionLenght; 

		if (VerQueryValue(pVersion, L"\\", (LPVOID*)&pVersionInfo, &u32VersionLenght)) 
			strVersion = QString::number(HIWORD(pVersionInfo->dwFileVersionMS)) + "." + QString::number(LOWORD(pVersionInfo->dwFileVersionMS)) 
				+ "." + QString::number(HIWORD(pVersionInfo->dwFileVersionLS)) + " (" + QString::number(LOWORD(pVersionInfo->dwFileVersionLS)) + ")";
	}

	::VirtualFree(pVersion, 0, MEM_RELEASE);
	return strVersion;
}

#endif
