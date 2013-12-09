#include "WindowsUtils.h"
#include "utils/win32/PipeDispatcher.h"

TrayNotificationReceiver::TrayNotificationReceiver(ITrayNotify *m_ITrayNotify, QString &executable)
{
    this->m_ITrayNotify = m_ITrayNotify;
    this->m_ITrayNotifyNew = NULL;
    this->executable = executable;
    m_cRef = 0;
}

TrayNotificationReceiver::TrayNotificationReceiver(ITrayNotifyNew *m_ITrayNotifyNew, QString &executable)
{
    this->m_ITrayNotifyNew = m_ITrayNotifyNew;
    this->m_ITrayNotify = NULL;
    this->executable = executable;
    m_cRef = 0;
}

boolean TrayNotificationReceiver::start()
{
    if(m_ITrayNotify) return m_ITrayNotify->RegisterCallback (this);
    else if(m_ITrayNotifyNew) return m_ITrayNotifyNew->RegisterCallback (this, &id);
    return false;
}

void TrayNotificationReceiver::stop()
{
    if(m_ITrayNotify)
    {
        m_ITrayNotify->Release();
        m_ITrayNotify = NULL;
    }
    else if(m_ITrayNotifyNew)
    {
        m_ITrayNotifyNew->UnregisterCallback(id);
        m_ITrayNotifyNew->Release();
        m_ITrayNotifyNew = NULL;
    }
}

TrayNotificationReceiver::~TrayNotificationReceiver()
{
    stop();
}

HRESULT __stdcall TrayNotificationReceiver::QueryInterface(REFIID riid, PVOID *ppv)
{
    if (ppv == NULL) return E_POINTER;

    if (riid == __uuidof (INotificationCB))
        *ppv = (INotificationCB *) this;
    else if (riid == IID_IUnknown)
        *ppv = (IUnknown *) this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG __stdcall TrayNotificationReceiver::AddRef(VOID)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

ULONG __stdcall TrayNotificationReceiver::Release(VOID)
{
    ULONG ulRefCount = InterlockedDecrement(&m_cRef);
    if (m_cRef == 0)
        delete this;
    return ulRefCount;
}


HRESULT __stdcall TrayNotificationReceiver::Notify (ULONG Event,
                          NOTIFYITEM *NotifyItem)
{
    if((m_ITrayNotify == NULL) && (m_ITrayNotifyNew==NULL)) return S_OK;
    if(Event != 0)
    {
        stop();
        return S_OK;
    }

    QString name = QString::fromWCharArray(NotifyItem->pszExeName);
    if(name.contains(executable))
    {
        cout << "TRAYICON Found!!" << endl;
        NotifyItem->dwPreference = 2;
        if(m_ITrayNotify) m_ITrayNotify->SetPreference(NotifyItem);
        else if(m_ITrayNotifyNew) m_ITrayNotifyNew->SetPreference(NotifyItem);
    }

    return S_OK;
}

boolean WindowsUtils::enableIcon(QString &executable)
{
    HRESULT hr;
    ITrayNotify *m_ITrayNotify;
    ITrayNotifyNew *m_ITrayNotifyNew;

    CoInitialize(NULL);
    hr = CoCreateInstance (
          __uuidof (TrayNotify),
          NULL,
          CLSCTX_LOCAL_SERVER,
          __uuidof (ITrayNotify),
          (PVOID *) &m_ITrayNotify);
    if(hr != S_OK)
    {
        cout << "ITrayNotify failed. Trying ITrayNotifyNew..." << endl;
        hr = CoCreateInstance (
          __uuidof (TrayNotify),
          NULL,
          CLSCTX_LOCAL_SERVER,
          __uuidof (ITrayNotifyNew),
          (PVOID *) &m_ITrayNotifyNew);
        if(hr != S_OK) return false;
    }

    TrayNotificationReceiver *receiver = NULL;
    if(m_ITrayNotify) receiver = new TrayNotificationReceiver(m_ITrayNotify, executable);
    else receiver = new TrayNotificationReceiver(m_ITrayNotifyNew, executable);

    hr = receiver->start();
    if(hr != S_OK) return false;

    return true;
}

void WindowsUtils::notifyItemChange(QString &path)
{
	wchar_t windowsPath[MAX_PATH];
	int len = path.toWCharArray(windowsPath);
	windowsPath[len]=L'\0';
	SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, windowsPath, NULL);
}

#include <QDebug>

//From http://msdn.microsoft.com/en-us/library/windows/desktop/bb776891.aspx
HRESULT WindowsUtils::CreateLink(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);

        // Query IShellLink for the IPersistFile interface, used for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        {
            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

QHash<QString, QString> WindowsUtils::extensionIcons;
void WindowsUtils::initialize()
{
    extensionIcons["3ds"] = extensionIcons["3dm"]  = extensionIcons["max"] =
							extensionIcons["obj"]  = "3D.png";

	extensionIcons["aep"] = extensionIcons["aet"]  = "aftereffects.png";

    extensionIcons["mp3"] = extensionIcons["wav"]  = extensionIcons["3ga"]  =
                            extensionIcons["aif"]  = extensionIcons["aiff"] =
                            extensionIcons["flac"] = extensionIcons["iff"]  =
							extensionIcons["m4a"]  = extensionIcons["wma"]  =  "audio.png";

	extensionIcons["dxf"] = extensionIcons["dwg"] =  "cad.png";


    extensionIcons["zip"] = extensionIcons["rar"] = extensionIcons["tgz"]  =
                            extensionIcons["gz"]  = extensionIcons["bz2"]  =
                            extensionIcons["tbz"] = extensionIcons["tar"]  =
							extensionIcons["7z"]  = extensionIcons["sitx"] =  "compressed.png";

    extensionIcons["sql"] = extensionIcons["accdb"] = extensionIcons["db"]  =
                            extensionIcons["dbf"]  = extensionIcons["mdb"]  =
							extensionIcons["pdb"] = "database.png";

	extensionIcons["dwt"] = "dreamweaver.png";

    extensionIcons["xls"] = extensionIcons["xlsx"] = extensionIcons["xlt"]  =
							extensionIcons["xltm"]  = "excel.png";

    extensionIcons["exe"] = extensionIcons["com"] = extensionIcons["bin"]  =
                            extensionIcons["apk"]  = extensionIcons["app"]  =
                             extensionIcons["msi"]  = extensionIcons["cmd"]  =
							extensionIcons["gadget"] = "executable.png";

	extensionIcons["as"] = extensionIcons["ascs"] =
			   extensionIcons["asc"]  = "fla_lang.png";
	extensionIcons["fla"] = "flash.png";
	extensionIcons["flv"] = "flash_video.png";

    extensionIcons["fnt"] = extensionIcons["otf"] = extensionIcons["ttf"]  =
							extensionIcons["fon"]  = "font.png";

	extensionIcons["gpx"] = extensionIcons["kml"] = extensionIcons["kmz"]  = "gis.png";


    extensionIcons["gif"] = extensionIcons["tiff"]  = extensionIcons["tif"]  =
                            extensionIcons["bmp"]  = extensionIcons["png"] =
							extensionIcons["tga"]  = "graphic.png";

    extensionIcons["html"] = extensionIcons["htm"]  = extensionIcons["dhtml"]  =
							extensionIcons["xhtml"]  = "html.png";

	extensionIcons["ai"] = extensionIcons["ait"] = "illustrator.png";
	extensionIcons["jpg"] = extensionIcons["jpeg"] = "image.png";
	extensionIcons["indd"] = "indesign.png";

	extensionIcons["jar"] = extensionIcons["java"]  = extensionIcons["class"]  = "java.png";

	extensionIcons["mid"] = extensionIcons["midi"] = "midi.png";
	extensionIcons["pdf"] = "pdf.png";
    extensionIcons["abr"] = extensionIcons["psb"]  = extensionIcons["psd"]  =
							"photoshop.png";

    extensionIcons["pls"] = extensionIcons["m3u"]  = extensionIcons["asx"]  =
							"playlist.png";
	extensionIcons["pcast"] = "podcast.png";

	extensionIcons["pps"] = extensionIcons["ppt"]  = extensionIcons["pptx"] = "powerpoint.png";

	extensionIcons["prproj"] = extensionIcons["ppj"]  = "premiere.png";

    extensionIcons["3fr"] = extensionIcons["arw"]  = extensionIcons["bay"]  =
                            extensionIcons["cr2"]  = extensionIcons["dcr"] =
                            extensionIcons["dng"] = extensionIcons["fff"]  =
                            extensionIcons["mef"] = extensionIcons["mrw"]  =
                            extensionIcons["nef"] = extensionIcons["pef"]  =
                            extensionIcons["rw2"] = extensionIcons["srf"]  =
                            extensionIcons["orf"] = extensionIcons["rwl"]  =
							"raw.png";

    extensionIcons["rm"] = extensionIcons["ra"]  = extensionIcons["ram"]  =
							"real_audio.png";

    extensionIcons["sh"] = extensionIcons["c"]  = extensionIcons["cc"]  =
                            extensionIcons["cpp"]  = extensionIcons["cxx"] =
                            extensionIcons["h"] = extensionIcons["hpp"]  =
							extensionIcons["dll"] = "source_code.png";

    extensionIcons["ods"]  = extensionIcons["ots"]  =
                            extensionIcons["gsheet"]  = extensionIcons["nb"] =
                            extensionIcons["xlr"] = extensionIcons["numbers"]  =
							"spreadsheet.png";

	extensionIcons["swf"] = "swf.png";
	extensionIcons["torrent"] = "torrent.png";

    extensionIcons["txt"] = extensionIcons["rtf"]  = extensionIcons["ans"]  =
                            extensionIcons["ascii"]  = extensionIcons["log"] =
                            extensionIcons["odt"] = extensionIcons["wpd"]  =
							"text.png";

	extensionIcons["vcf"] = "vcard.png";
    extensionIcons["svgz"]  = extensionIcons["svg"]  =
                            extensionIcons["cdr"]  = extensionIcons["eps"] =
							"vector.png";


     extensionIcons["mkv"]  = extensionIcons["webm"]  =
                            extensionIcons["avi"]  = extensionIcons["mp4"] =
                            extensionIcons["m4v"] = extensionIcons["mpg"]  =
                            extensionIcons["mpeg"] = extensionIcons["mov"]  =
                            extensionIcons["3g2"] = extensionIcons["3gp"]  =
                            extensionIcons["asf"] = extensionIcons["wmv"]  =
							"video.png";


	 extensionIcons["srt"] = "video_subtitles.png";
	 extensionIcons["vob"] = "video_vob.png";

     extensionIcons["html"]  = extensionIcons["xml"] = extensionIcons["shtml"]  =
                            extensionIcons["dhtml"] = extensionIcons["js"] =
							extensionIcons["css"]  = "web_data.png";

     extensionIcons["php"]  = extensionIcons["php3"]  =
                            extensionIcons["php4"]  = extensionIcons["php5"] =
                            extensionIcons["phtml"] = extensionIcons["inc"]  =
                            extensionIcons["asp"] = extensionIcons["pl"]  =
                            extensionIcons["cgi"] = extensionIcons["py"]  =
							"web_lang.png";


     extensionIcons["doc"]  = extensionIcons["docx"] = extensionIcons["dotx"]  =
							extensionIcons["wps"] = "word.png";
}

bool WindowsUtils::startOnStartup(bool value)
{
    WCHAR path[MAX_PATH];
    HRESULT res = SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, path);
    if(res != S_OK)
        return false;

    QString startupPath = QString::fromWCharArray(path);
	startupPath += "\\MEGAsync.lnk";

    if(value)
    {
        WCHAR executablePath[MAX_PATH];
		WCHAR description[]=L"Start MEGAsync";

        int len = startupPath.toWCharArray(path);
        path[len] = L'\0';

        QString exec = QCoreApplication::applicationFilePath();
		exec = QDir::toNativeSeparators(exec);
        len = exec.toWCharArray(executablePath);
        executablePath[len]= L'\0';

        res = CreateLink(executablePath, path, description);
        if(res != S_OK)
            return false;

        return true;
    }
    else
    {
        QFile::remove(startupPath);
        return true;
	}
}

void WindowsUtils::showInFolder(QString pathIn)
{
	#if defined(Q_OS_WIN)
		QString param;
		param = QLatin1String("/select,");
		param += "\"\"" + QDir::toNativeSeparators(pathIn) + "\"\"";
		QProcess::startDetached("explorer " + param);
	#elif defined(Q_OS_MAC)
		Q_UNUSED(parent)
		QStringList scriptArgs;
		scriptArgs << QLatin1String("-e")
				   << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
										 .arg(pathIn);
		QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
		scriptArgs.clear();
		scriptArgs << QLatin1String("-e")
				   << QLatin1String("tell application \"Finder\" to activate");
		QProcess::execute("/usr/bin/osascript", scriptArgs);
	#else
		// we cannot select a file here, because no file browser really supports it...
		const QFileInfo fileInfo(pathIn);
		const QString folder = fileInfo.absoluteFilePath();
		const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::instance()->settings());
		QProcess browserProc;
		const QString browserArgs = Utils::UnixUtils::substituteFileBrowserParameters(app, folder);
		if (debug)
			qDebug() <<  browserArgs;
		bool success = browserProc.startDetached(browserArgs);
		const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
		success = success && error.isEmpty();
		if (!success)
			showGraphicalShellError(parent, app, error);
#endif
}

void WindowsUtils::countFilesAndFolders(QString path, long *numFiles, long *numFolders)
{
	QFileInfo baseDir(path);
	if(!baseDir.exists() || !baseDir.isDir()) return;
	if(((*numFolders) > 500) || ((*numFiles) > 20000)) return;

	QDir dir(path);
	QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
	for(int i=0; i<entries.size(); i++)
	{
		QFileInfo info = entries[i];
		if(info.isFile()) (*numFiles)++;
		else if(info.isDir())
		{
			countFilesAndFolders(info.absoluteFilePath(), numFiles, numFolders);
			(*numFolders)++;
		}
	}
}


WindowsUtils::WindowsUtils()
{
}

QString WindowsUtils::getSizeString(unsigned long long bytes)
{
	unsigned long long KB = 1024;
	unsigned long long MB = 1024 * KB;
	unsigned long long GB = 1024 * MB;
	unsigned long long TB = 1024 * GB;

	if(bytes > TB)
		return QString::number( ((int)((100 * bytes) / TB))/100.0) + " TB";

	if(bytes > GB)
		return QString::number( ((int)((100 * bytes) / GB))/100.0) + " GB";

	if(bytes > MB)
		return QString::number( ((int)((100 * bytes) / MB))/100.0) + " MB";

	if(bytes > KB)
		return QString::number( ((int)((100 * bytes) / KB))/100.0) + " KB";

	return QString::number(bytes) + " bytes";
}


