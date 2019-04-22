#include <Windows.h>
#include <InitGuid.h>
#include "gpedit.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <xlocmon>
#include "combaseapi.h"

#pragma comment(lib, "Ole32.lib")

#define DISALLOW_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun"
#define EXPLORER_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"

HKEY get_hk()
{
	HKEY hk;
	LPCSTR sk = TEXT(DISALLOW_PATH);
	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hk);

	if (openRes != ERROR_SUCCESS)
	{
		if (openRes == 2)
		{
			sk = TEXT(EXPLORER_PATH);
			openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hk);
			if (openRes != ERROR_SUCCESS)
			{
				std::cerr << "Error while opening reg key" << std::endl;
				exit(1);
			}
			std::cout << "Policy not configured, configuring...";
			DWORD one = 1;
			LONG setRes = RegSetValueEx(hk, "DisallowRun", 0, REG_DWORD, (const BYTE*) &one, sizeof(DWORD));
			RegCreateKey(hk, "DisallowRun", &hk);
			std::cout << "done" << std::endl;
		}
		else
		{

			std::cerr << "Error while opening reg key" << std::endl;
			exit(1);
		}
	}

	return hk;
}

unsigned long get_num_of_values(HKEY hk)
{

	DWORD num_of_values = 0;
	LONG infoRes = RegQueryInfoKey(hk, NULL, NULL, NULL, NULL, NULL, NULL, &num_of_values, NULL, NULL, NULL, NULL);

	if (infoRes != ERROR_SUCCESS)
	{
		std::cerr << "Error while getting num of values" << std::endl;
		return -1;
	}

	return num_of_values;
}

LPCSTR get_val_from_i(unsigned long i)
{

	char val[15];
	std::string str = std::to_string(i);
	LPCSTR value = TEXT(str.c_str());
	return value;

}

int set_new_value(HKEY hk, unsigned long val, LPCSTR data)
{
	LPCSTR value = get_val_from_i(val);
	std::cout << "New value " << std::to_string(val).c_str() << " : " << data << std::endl;
	LONG setRes = RegSetValueEx(hk, std::to_string(val).c_str(), 0, REG_SZ, (LPBYTE)data, strlen(data) + 1);

	

	if (setRes != ERROR_SUCCESS)
	{
		std::cerr << "Error while writing to reg" << std::endl;
		return -1;
	}
}

int close_reg(HKEY hk)
{
	LONG closeOut = RegCloseKey(hk);

	if (closeOut != ERROR_SUCCESS)
	{
		std::cerr << "Error while closing reg" << std::endl;
		return -1;
	}
}

std::vector<char*> get_restrictions(char* filename)
{
	std::ifstream f(filename);
	FILE* fi;
	fopen_s(&fi, filename, "r");
	if (!f.is_open())
	{
		std::cerr << "Problem with file " << filename << std::endl;
		std::vector<char*> restrictions;
		return restrictions;
	}
	std::vector<char*> restrictions;
	std::string str;
	while(std::getline(f, str))
	{
		//std::cout << str << std::endl;
		char* cstr = (char*)calloc(256, sizeof(char));
		memcpy(cstr, str.c_str(), 256);
		//std::cout << cstr << " : ";
		cstr[str.length()] = 0;
		//std::cout << cstr << std::endl;
		restrictions.push_back(cstr);
	}
	//std::cout << restrictions.size() << std::endl;
	return restrictions;
}

HRESULT ModifyUserPolicyForDisallowRunningSpecifiedWindowsApplication(BSTR bGPOPath, int iMode, DWORD lData);

int oldmain(int argc, char** argv) 
{
	if (argc != 2)
	{
		std::cout << "*.exe restrictions.txt" << std::endl;
		return 0;
	}
	std::vector<char*> rests = get_restrictions(argv[1]);

	HKEY hKey = get_hk();

	DWORD lData = 1;

	ModifyUserPolicyForDisallowRunningSpecifiedWindowsApplication(NULL, 1, lData);

	unsigned long valnum = get_num_of_values(hKey);

	for (int i = 0; i < rests.size(); i++)
	{
		set_new_value(hKey, i + valnum + 1, rests[i]);
		free(rests[i]);
	}




	close_reg(hKey);
}

HRESULT ModifyUserPolicyForDisallowRunningSpecifiedWindowsApplication(BSTR bGPOPath, int iMode, DWORD lData)
{
	BSTR bGPOPath_l = NULL;
	HRESULT hr = S_OK;
	IGroupPolicyObject* p = NULL;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_INPROC_SERVER, IID_IGroupPolicyObject, (LPVOID*)&p);

	if (hr != S_OK)
	{
		char buff[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), NULL, buff, 1024, NULL);
		std::cout << buff << std::endl;
		exit(-1);
	}

	if (SUCCEEDED(hr))
	{
		DWORD dwSection = GPO_SECTION_USER;
		HKEY hGPOSectionKey = NULL;
		DWORD dwData;
		HKEY hSettingKey;
		LSTATUS rStatus;
		hr = 0;

		char bufff[1024];


		hr = p->OpenLocalMachineGPO(GPO_OPEN_LOAD_REGISTRY);

		if (hr != S_OK)
		{
			char buff[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), NULL, buff, 1024, NULL);
			std::cout << buff << std::endl;
			exit(-1);
		}

		hr = p->GetRegistryKey(dwSection, &hGPOSectionKey);

		if (hr != S_OK)
		{
			char buff[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), NULL, buff, 1024, NULL);
			std::cout << buff << std::endl;
			exit(-1);
		}

		//iMode:
		//0=Not Configured, 1=Enabled, 2=Disabled

		switch(iMode)
		{
		case 0:
			rStatus = RegDeleteValue(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun");

			if (rStatus != S_OK)
			{
				char buff[1024];
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), NULL, buff, 1024, NULL);
				std::cout << buff << std::endl;
				//exit(-1);
			}

			rStatus = RegDeleteKey(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun");
			if (rStatus != S_OK)
			{
				char buff[1024];
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), NULL, buff, 1024, NULL);
				std::cout << buff << std::endl;
				std::cout << buff << std::endl;
				//exit(-1);
			}
			break;
		case 1: 
			if (RegOpenKeyEx(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, KEY_WRITE, &hSettingKey) != ERROR_SUCCESS)
			{
				rStatus = RegCreateKeyEx(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSettingKey, NULL);
			}


			rStatus = RegSetValueEx(hSettingKey, "DisallowRun", NULL, REG_DWORD, (BYTE*)(&lData), sizeof(DWORD));
			rStatus = RegCreateKeyEx(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallRun", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL, NULL);
			rStatus = RegCloseKey(hSettingKey);
			break; 
		
		case 2:
			bool bCreate = false;

			if (RegOpenKeyEx(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, KEY_WRITE, &hSettingKey) != ERROR_SUCCESS)
			{
				rStatus = RegCreateKeyEx(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSettingKey, NULL);
				bCreate = true;
			}

			DWORD dwType = 0;
			DWORD cbType = sizeof(dwData);
			dwData = 0;
			if (!bCreate)
			{
				rStatus = RegGetValue(hGPOSectionKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "DisallowRun", RRF_RT_ANY, &dwType, (BYTE*)(&dwData), &cbType);
				if (rStatus != ERROR_SUCCESS)
				{
					dwData = 0; 
				}
				else
				{
					RegDeleteValue(hSettingKey, "DisallowRun");
				}
				dwData = 0; //на всякий случай.
				rStatus = RegSetValueEx(hSettingKey, "DisallowRun", NULL, REG_DWORD, (BYTE*)(&dwData), sizeof(DWORD));
			}
			else
			{
				rStatus = RegSetValueEx(hSettingKey, "DisallowRun", NULL, REG_DWORD, (BYTE*)(&dwData), sizeof(DWORD));
			}
			rStatus = RegCloseKey(hSettingKey);

		}


		GUID RegistryId = REGISTRY_EXTENSION_GUID;
		GUID ThisAdminToolGuid = { 0x0F6B957E,0x509E,0x11D1,{0xA7, 0xCC, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xE3} };
		rStatus = RegCloseKey(hGPOSectionKey);

		hr = p->Save(FALSE, TRUE, &RegistryId, &ThisAdminToolGuid);
		hr = p->Release();
	}
	else
	{
		switch(hr)
		{
		case S_OK: std::cout << "OK" << std::endl; break;
		case REGDB_E_CLASSNOTREG: std::cout << "REGDB_E_CLASSNOTREG" << std::endl; break;
		case CLASS_E_NOAGGREGATION: std::cout << "CLASS_E_NOAGGREGATION" << std::endl; break;
		case E_NOINTERFACE:std::cout << "E_NOINTERFACE" << std::endl; break;
		case E_POINTER:std::cout << "E_POINTER" << std::endl; break;
		}
		
	}
	return hr;
}

int main(int argc, char** argv) {

	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	if (argc == 2 && !strcmp(argv[1], "delete"))
	{
		ModifyUserPolicyForDisallowRunningSpecifiedWindowsApplication(NULL, 0, 0);
		exit(0);
	}
	if (argc == 2 && !strcmp(argv[1], "disable"))
	{
		ModifyUserPolicyForDisallowRunningSpecifiedWindowsApplication(NULL, 2, 0);
		exit(0);
	}
	if (argc == 2 && !strcmp(argv[1], "enable"))
	{
		ModifyUserPolicyForDisallowRunningSpecifiedWindowsApplication(NULL, 1, 1);
		exit(0);
	}


	oldmain(argc, argv);

	return 0;
}