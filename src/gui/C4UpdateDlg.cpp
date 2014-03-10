/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
// dialogs for update, and the actual update application code
// is only compiled WITH_AUTOMATIC_UPDATE

#include "C4Include.h"

#include "C4UpdateDlg.h"

#include "C4DownloadDlg.h"
#include <C4Log.h>

#ifdef _WIN32
#include <shellapi.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int C4UpdateDlg::pid;
int C4UpdateDlg::c4group_output[2];
bool C4UpdateDlg::succeeded;

// --------------------------------------------------
// C4UpdateDlg

C4UpdateDlg::C4UpdateDlg() : C4GUI::InfoDialog(LoadResStr("IDS_TYPE_UPDATE"), 10)
{
	// initial text update
	UpdateText();
	// assume update running
	UpdateRunning = true;
}

void C4UpdateDlg::UserClose(bool fOK)
{
	// Update not done yet: can't close
	if (UpdateRunning)
	{
		GetScreen()->ShowMessage(LoadResStr("IDS_MSG_UPDATEINPROGRESS"), LoadResStr("IDS_TYPE_UPDATE"), C4GUI::Ico_Ex_Update);
		return;
	}
	// Okay to close
	Dialog::UserClose(fOK);
#ifndef _WIN32
	if (succeeded)
	{
		// ready for restart
		Application.restartAtEnd = true;
		Application.Quit();
	}
#endif
}

void C4UpdateDlg::UpdateText()
{
	if (!UpdateRunning)
		return;
#ifdef _WIN32
	AddLine("Win32 is currently not using the C4UpdateDlg!");
	UpdateRunning = false;
#else
	char c4group_output_buf[513];
	ssize_t amount_read;
	// transfer output to log window
	amount_read = read(c4group_output[0], c4group_output_buf, 512);
	// error
	if (amount_read == -1)
	{
		if (errno == EAGAIN)
			return;
		StdStrBuf Errormessage = FormatString("read error from c4group: %s", strerror(errno));
		Log(Errormessage.getData());
		AddLine(Errormessage.getData());
		UpdateRunning = false;
		succeeded = false;
	}
	// EOF: Update done.
	else if (amount_read == 0)
	{
		// Close c4group output
		close(c4group_output[0]);
		// If c4group did not exit but something else caused EOF, then that's bad. But don't hang.
		int child_status = 0;
		if (waitpid(pid, &child_status, WNOHANG) == -1)
		{
			LogF("error in waitpid: %s", strerror(errno));
			AddLineFmt("error in waitpid: %s", strerror(errno));
			succeeded = false;
		}
		// check if c4group failed.
		else if (WIFEXITED(child_status) && WEXITSTATUS(child_status))
		{
			LogF("c4group returned status %d", WEXITSTATUS(child_status));
			AddLineFmt("c4group returned status %d", WEXITSTATUS(child_status));
			succeeded = false;
		}
		else if (WIFSIGNALED(child_status))
		{
			LogF("c4group killed with signal %d", WTERMSIG(child_status));
			AddLineFmt("c4group killed with signal %d", WTERMSIG(child_status));
			succeeded = false;
		}
		else
		{
			Log("Done.");
			AddLine("Done.");
		}
		UpdateRunning = false;
	}
	else
	{
		c4group_output_buf[amount_read] = 0;
		// Fixme: This adds spurious newlines in the middle of the output.
		LogF("%s", c4group_output_buf);
		AddLineFmt("%s", c4group_output_buf);
	}
#endif

	// Scroll to bottom
	if (pTextWin)
	{
		pTextWin->UpdateHeight();
		pTextWin->ScrollToBottom();
	}
}

// --------------------------------------------------
// static update application function

void C4UpdateDlg::RedirectToDownloadPage()
{
	OpenURL("http://www.openclonk.org/download");
}

bool C4UpdateDlg::DoUpdate(const char *szUpdateURL, C4GUI::Screen *pScreen)
{
	if(szUpdateURL == NULL || strlen(szUpdateURL) == 0)
	{
		pScreen->ShowMessageModal(LoadResStr("IDS_MSG_NEWRELEASEAVAILABLE"), LoadResStr("IDS_TYPE_UPDATE"),C4GUI::MessageDialog::btnOK, C4GUI::Ico_Ex_Update);
		RedirectToDownloadPage();
		return true;
	}

	// Determine local filename for update group
	StdCopyStrBuf strLocalFilename(Config.AtTempPath(GetFilename(szUpdateURL)));
	StdCopyStrBuf strRemoteURL(szUpdateURL);
	// cut off http://
	strRemoteURL.Replace("http://","");
	// Download update group
	if (!C4DownloadDlg::DownloadFile(LoadResStr("IDS_TYPE_UPDATE"), pScreen, strRemoteURL.getData(), strLocalFilename.getData(), LoadResStr("IDS_MSG_UPDATENOTAVAILABLE")))
	{
		// Download failed, open browser so the user can download a full package
		RedirectToDownloadPage();
		// return success, because error message has already been shown
		return true;
	}

	// Apply downloaded update
	return ApplyUpdate(strLocalFilename.getData(), true, pScreen);
}

bool C4UpdateDlg::ApplyUpdate(const char *strUpdateFile, bool fDeleteUpdate, C4GUI::Screen *pScreen)
{
	// Determine name of update program
	StdStrBuf strUpdateProg; strUpdateProg.Copy(C4CFN_UpdateProgram);
	// Windows: manually append extension because ExtractEntry() cannot properly glob and Extract() doesn't return failure values
#ifdef _WIN32
	strUpdateProg += ".exe";
#endif
	// Determine name of local extract of update program
	StdStrBuf strUpdateProgEx;
	strUpdateProgEx.Copy(Config.AtExePath(strUpdateProg.getData()));

	// Extract update program (the update should be applied using the new version)
	C4Group UpdateGroup;
	if (!UpdateGroup.Open(strUpdateFile))
	{
		LogF("Error opening \"%s\": %s", strUpdateFile, UpdateGroup.GetError());
		return false;
	}
	// Look for update program at top level
	if (!UpdateGroup.ExtractEntry(strUpdateProg.getData(), strUpdateProgEx.getData()))
	{
		LogF("Error extracting \"%s\": %s", strUpdateProg.getData(), UpdateGroup.GetError());
		return false;
	}
#if 0
	char strSubGroup[1024+1];
		// ASK: What is this? Why should an update program not be found at the top
		// level? This seems obsolete. - Newton
		// Not found: look for an engine update pack one level down
		if (UpdateGroup.FindEntry(FormatString("cr_*_%s.ocu", C4_OS).getData(), strSubGroup))
			// Extract update program from sub group
			if (SubGroup.OpenAsChild(&UpdateGroup, strSubGroup))
			{
				SubGroup.ExtractEntry(strUpdateProg.getData(), strUpdateProgEx.getData());
				SubGroup.Close();
			}
#endif
	UpdateGroup.Close();

	// Execute update program
	Log(LoadResStr("IDS_PRC_LAUNCHINGUPDATE"));
	succeeded = true;

#ifdef _WIN32
	// Notice: even if the update program and update group are in the temp path, they must be executed in our working directory
	DWORD ProcessID = GetCurrentProcessId();
	StdStrBuf strUpdateArgs;
	strUpdateArgs.Format("\"%s\" \"%s\" %s %lu", strUpdateProgEx.getData(), strUpdateFile, fDeleteUpdate ? "-yd" : "-y", (unsigned long)ProcessID);

	// the magic verb "runas" opens the update program in a shell requesting elevation
	int iError = (intptr_t)ShellExecute(NULL, L"runas", strUpdateProgEx.GetWideChar(), strUpdateArgs.GetWideChar(), Config.General.ExePath.GetWideChar(), SW_SHOW);
	if (iError <= 32) return false;

	// must quit ourselves for update program to work
	if (succeeded) Application.Quit();
#else
	if (pipe(c4group_output) == -1)
	{
		Log("Error creating pipe");
		return false;
	}
	switch (pid = fork())
	{
		// Error
	case -1:
		Log("Error creating update child process.");
		return false;
		// Child process
	case 0:
		// Close unused read end
		close(c4group_output[0]);
		// redirect stdout and stderr to the parent
		dup2(c4group_output[1], STDOUT_FILENO);
		dup2(c4group_output[1], STDERR_FILENO);
		if (c4group_output[1] != STDOUT_FILENO && c4group_output[1] != STDERR_FILENO)
			close(c4group_output[1]);
		execl(C4CFN_UpdateProgram, C4CFN_UpdateProgram, "-v", strUpdateFile, (fDeleteUpdate ? "-yd" : "-y"), static_cast<char *>(0));
		printf("execl failed: %s\n", strerror(errno));
		exit(1);
		// Parent process
	default:
		// Close unused write end
		close(c4group_output[1]);
		// disable blocking
		fcntl(c4group_output[0], F_SETFL, O_NONBLOCK);
		// Open the update log dialog (this will update itself automatically from c4group_output)
		pScreen->ShowRemoveDlg(new C4UpdateDlg());
		break;
	}
#endif
	// done
	return succeeded;
}

bool C4UpdateDlg::IsValidUpdate(const char *szVersion)
{
	StdStrBuf strVersion; strVersion.Format("%d.%d.%d", C4XVER1, C4XVER2, C4XVER3);
	if (szVersion == NULL || strlen(szVersion) == 0) return false;
	return strcmp(szVersion,strVersion.getData()) != 0;
}

bool C4UpdateDlg::CheckForUpdates(C4GUI::Screen *pScreen, bool fAutomatic)
{
	// Automatic update only once a day
	if (fAutomatic)
		if (time(NULL) - Config.Network.LastUpdateTime < 60 * 60 * 24)
			return false;
	// Store the time of this update check (whether it's automatic or not or successful or not)
	Config.Network.LastUpdateTime = time(NULL);
	// Get current update url and version info from server
	StdStrBuf UpdateURL;
	StdStrBuf VersionInfo;
	C4GUI::Dialog *pWaitDlg = NULL;
	pWaitDlg = new C4GUI::MessageDialog(LoadResStr("IDS_MSG_LOOKINGFORUPDATES"), LoadResStr("IDS_TYPE_UPDATE"), C4GUI::MessageDialog::btnAbort, C4GUI::Ico_Ex_Update, C4GUI::MessageDialog::dsRegular);
	pWaitDlg->SetDelOnClose(false);
	pScreen->ShowDialog(pWaitDlg, false);

	C4Network2UpdateClient UpdateClient;
	bool fSuccess = false, fAborted = false;
	StdStrBuf strVersion; strVersion.Format("%d.%d.%d", C4XVER1, C4XVER2, C4XVER3);
	StdStrBuf strQuery; strQuery.Format("%s?version=%s&platform=%s&action=version", Config.Network.UpdateServerAddress, strVersion.getData(), C4_OS);
	if (UpdateClient.Init() && UpdateClient.SetServer(strQuery.getData()) && UpdateClient.QueryUpdateURL())
	{
		UpdateClient.SetNotify(&Application.InteractiveThread);
		Application.InteractiveThread.AddProc(&UpdateClient);
		// wait for version check to terminate
		while (UpdateClient.isBusy())
		{
			// wait, check for program abort
			if (!Application.ScheduleProcs()) { fAborted = true; break; }
			// check for dialog close
			if (pWaitDlg) if (!pWaitDlg->IsShown())  { fAborted = true; break; }
		}
		if (!fAborted) 
		{
			fSuccess = UpdateClient.GetVersion(&VersionInfo);
			UpdateClient.GetUpdateURL(&UpdateURL);
		}
		Application.InteractiveThread.RemoveProc(&UpdateClient);
		UpdateClient.SetNotify(NULL);
	}
	delete pWaitDlg;
	// User abort
	if (fAborted)
	{
		return false;
	}
	// Error during update check
	if (!fSuccess)
	{
		StdStrBuf sError; sError.Copy(LoadResStr("IDS_MSG_UPDATEFAILED"));
		const char *szErrMsg = UpdateClient.GetError();
		if (szErrMsg)
		{
			sError.Append(": ");
			sError.Append(szErrMsg);
		}
		pScreen->ShowMessage(sError.getData(), LoadResStr("IDS_TYPE_UPDATE"), C4GUI::Ico_Ex_Update);
		return false;
	}
	// Applicable update available
	if (C4UpdateDlg::IsValidUpdate(VersionInfo.getData()))
	{
		// Prompt user, then apply update
		if (pScreen->ShowMessageModal(LoadResStr("IDS_MSG_ANUPDATETOVERSIONISAVAILA"), LoadResStr("IDS_TYPE_UPDATE"), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_Ex_Update))
		{
			if (!DoUpdate(UpdateURL.getData(), pScreen))
				pScreen->ShowMessage(LoadResStr("IDS_MSG_UPDATEFAILED"), LoadResStr("IDS_TYPE_UPDATE"), C4GUI::Ico_Ex_Update);
			else
				return true;
		}
	}
	// No applicable update available
	else
	{
		// Message (if not automatic)
		if (!fAutomatic)
			pScreen->ShowMessage(LoadResStr("IDS_MSG_NOUPDATEAVAILABLEFORTHISV"), LoadResStr("IDS_TYPE_UPDATE"), C4GUI::Ico_Ex_Update);
	}
	// Done (and no update has been done)
	return false;
}
