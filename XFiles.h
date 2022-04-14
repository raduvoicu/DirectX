//Wrapper Class to demonstrate X Files

//CSaveDataObject encapsulates ID3DXFileSaveData
//CDataObject encapsulates ID3DXFileData
//CXFile encapsulates ID3DXFile and ID3DXFileEnumObject

//To use these classes, simply include this file as a header in your projects

//Example:

//CXFile *File = new CXFile();
//File->LoadFromFile("File.x");

//SIZE_T Count = File->GetChildCount();

//for(SIZE_T Counter = 0; Counter < Count; Counter++)
//{
//	CDataObject *Object = File->GetChild(Counter);

	//Do Stuff Here

//	delete Object;
//}

#include <dxfile.h>
#include <initguid.h>
#include <rmxftmpl.h>
#include <rmxfguid.h>

bool FileExists(char* File)
{
	WIN32_FIND_DATA WinFindData;
	ZeroMemory(&WinFindData, sizeof(WIN32_FIND_DATA));

	HANDLE hSearchHandle = FindFirstFile(File, &WinFindData);

	if (hSearchHandle == INVALID_HANDLE_VALUE)
		return false;
	else
	{
		FindClose(hSearchHandle);
		return true;
	}
}

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

#include <dxfile.h>
#include <rmxfguid.h>
#include <rmxftmpl.h>
#include <initguid.h>

class CSaveDataObject
{
private:
protected:
	LPD3DXFILESAVEDATA m_pSaveData;

	VOID CleanUp()
	{
		SAFE_RELEASE(m_pSaveData);
	}

public:
	CSaveDataObject(LPD3DXFILESAVEDATA Data)
	{
		m_pSaveData = Data;
	}

	~CSaveDataObject()
	{
		CleanUp();
	}
	
	LPD3DXFILESAVEDATA GetSaveDataInterface() const {return m_pSaveData;}

	CSaveDataObject* AddChildSaveDataObject(REFGUID rguidTemplate, LPCSTR szName, const GUID *pId, SIZE_T cbSize, LPCVOID pvData)
	{
		if(!m_pSaveData)
			return NULL;

		LPD3DXFILESAVEDATA Data = NULL;

		if(FAILED(m_pSaveData->AddDataObject(rguidTemplate, szName, pId, cbSize, pvData, &Data)))
			return NULL;

		return new CSaveDataObject(Data);
	}

	HRESULT AddDataReference(LPCSTR szName, const GUID *pId)
	{
		if(!m_pSaveData)
			return NULL;

		return m_pSaveData->AddDataReference(szName, pId);
	}
};

//-----------------------------------------------------------------------------

class CDataObject
{
private:
protected:
		LPD3DXFILEDATA m_pData;
		char* m_Name;
		GUID m_ID;
		VOID *m_Data;
		BOOL m_IsReference;

		VOID CleanUp()
		{
			SAFE_RELEASE(m_pData);
		}

public:
		CDataObject(LPD3DXFILEDATA Data)
		{
			m_pData = Data;
			m_Name = NULL;
			m_IsReference = false;
		}

		~CDataObject()
		{
			if(m_Name)
				delete [] m_Name;

			CleanUp();
		}
		
		BOOL IsReference() const {return m_IsReference;}
		char* GetName() const {return m_Name;}
		GUID GetType() const {return m_ID;}
		LPD3DXFILEDATA GetDataInterface() const {return m_pData;}
		
		SIZE_T GetChildCount()
		{
			if(!m_pData)
				return 0;

			SIZE_T Size = 0;

			if(FAILED(m_pData->GetChildren(&Size)))
				return 0;

			return Size;
		}
		
		CDataObject* GetChild(SIZE_T Object)
		{
			if(!m_pData)
				return NULL;

			LPD3DXFILEDATA Data = NULL;

			if(FAILED(m_pData->GetChild(Object, &Data)))
				return NULL;

			return new CDataObject(Data);
		}

		HRESULT GetData()
		{
			if(!m_pData)
				return E_FAIL;

			SIZE_T Size;

			HRESULT Result = m_pData->GetName(NULL, &Size);

			if(FAILED(Result))
				return Result;

			if(m_Name)
				delete [] m_Name;

			m_Name = new char[Size];

			Result = m_pData->GetName(m_Name, &Size);

			if(FAILED(Result))
				return Result;
	
			Result = m_pData->GetType(&m_ID);

			if(FAILED(Result))
				return Result;

			m_IsReference = m_pData->IsReference();

			return S_OK;
		}

		HRESULT Lock(SIZE_T *Data, const VOID **ppData)
		{
			if(!m_pData)
				return E_FAIL;

			return m_pData->Lock(Data, ppData);
		}

		BOOL Unlock()
		{
			if(!m_pData)
				return false;

			return m_pData->Unlock();
		}
};

//-----------------------------------------------------------------------------

//Wraps an X File

class CXFile
{
	private:
	protected:
		LPD3DXFILE m_pFile;
		LPD3DXFILEENUMOBJECT m_pEnumObject;
		LPD3DXFILESAVEOBJECT m_pSave;

		VOID CleanUp()
		{
			SAFE_RELEASE(m_pFile);
			SAFE_RELEASE(m_pEnumObject);
			SAFE_RELEASE(m_pSave);
		}

	public:
		
		CXFile()
		{
			m_pFile = NULL;
			m_pEnumObject = NULL;
			m_pSave = NULL;
		}

		~CXFile()
		{
			CleanUp();
		}

		SIZE_T GetChildCount()
		{
			if(!m_pEnumObject)
				return 0;
		
			SIZE_T Count = 0;

			if(FAILED(m_pEnumObject->GetChildren(&Count)))
				return 0;

			return Count;
		}

		CDataObject* GetChild(SIZE_T Object)
		{
			if(!m_pEnumObject)
				return NULL;

			LPD3DXFILEDATA Data = NULL;

			if(FAILED(m_pEnumObject->GetChild(Object, &Data)))
				return NULL;

			return new CDataObject(Data);
		}

		HRESULT LoadFromFile(char* File)
		{
			if(!FileExists(File))
			return E_FAIL;

			HRESULT Result = D3DXFileCreate(&m_pFile);

			if(FAILED(Result))
				return Result;

			Result = m_pFile->RegisterTemplates((LPVOID)D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES);

			if(FAILED(Result))
			{
				CleanUp();
				return Result;
			}

			Result = m_pFile->CreateEnumObject(File, D3DXF_FILELOAD_FROMFILE, &m_pEnumObject);

			if(FAILED(Result))
			{
				CleanUp();
				return Result;
			}

			return S_OK;
		}

		HRESULT CreateFile(char* File)
		{
			HRESULT Result = D3DXFileCreate(&m_pFile);

			if(FAILED(Result))
				return Result;

			Result = m_pFile->RegisterTemplates((LPVOID)D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES);

			if(FAILED(Result))
			{
				CleanUp();
				return Result;
			}

			Result = m_pFile->CreateSaveObject(File, D3DXF_FILESAVE_TOFILE, D3DXF_FILEFORMAT_TEXT, &m_pSave);

			if(FAILED(Result))
			{
				CleanUp();
				return Result;
			}

			return S_OK;
		}

		CSaveDataObject* CreateSaveDataObject(REFGUID rguidTemplate, LPCSTR szName, const GUID *pId, SIZE_T cbSize, LPCVOID pvData)
		{
			if(!m_pSave)
				return NULL;

			LPD3DXFILESAVEDATA Data = NULL;

			if(FAILED(m_pSave->AddDataObject(rguidTemplate, szName, pId, cbSize, pvData, &Data)))
				return NULL;

			return new CSaveDataObject(Data);
		}

		HRESULT Save()
		{
			if(!m_pSave)
				return E_FAIL;

			return m_pSave->Save();
		}

		HRESULT RegisterTemplates(LPCVOID pvData, SIZE_T cbSize)
		{
			if(!m_pFile)
				return E_FAIL;

			return m_pFile->RegisterTemplates(pvData, cbSize);
		}
};
