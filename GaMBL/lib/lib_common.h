
// Common headers used by library

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef LIB_COMMON_H
#define LIB_COMMON_H

#ifndef TARGET_OS_IPHONE
    #include <Cocoa/Cocoa.h>
#endif

#include "blargg_common.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <string>

/*inline std::string& operator=(std::string & lhs, const std::string & rhs) {
    return lhs.assign(rhs);
}*/

//RAD
class GaMBLFileHandle
{
public:
    GaMBLFileHandle() : m_pHandle( NULL )
    {
    }
    
    GaMBLFileHandle( std::wstring& strPath, const char* pszMode )
    {
        OpenFileFromPath( strPath, pszMode );
    }
    
    bool OpenFileFromPath( std::wstring& strPath, const char* pszMode )
    {
        CloseFile();
        
        char szPath[PATH_MAX * sizeof(strPath[0])];
        const wchar_t* wcs = strPath.c_str();
        wcsrtombs( szPath, &wcs, sizeof(szPath), NULL );
        m_pHandle = fopen( szPath, pszMode );
        assert( m_pHandle );
        
        return m_pHandle != NULL;
    }
    
    virtual ~GaMBLFileHandle()
    {
        CloseFile();
    }
    
    int GetDescriptor() const
    {
        assert( m_pHandle );
        return fileno( m_pHandle );
    }
    
    int GetFilePath( std::wstring& strPath ) const
    {
        assert( m_pHandle );
        int fd = GetDescriptor();
        strPath.resize( PATH_MAX );
        int ret = fcntl( fd, F_GETPATH, strPath.begin() );
        return ret;
    }
    
    bool IsOk() const
    {
        return m_pHandle != NULL;
    }
    
private:
    void CloseFile()
    {
        fclose( m_pHandle );
    }
    FILE* m_pHandle;
};

extern GaMBLFileHandle DummyHandle;

#include "debug_out.h"
#include "util.h"
#include "error_util.h"
#include "runtime_array.h"

using namespace std;

#endif

