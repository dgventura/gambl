
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
    
    GaMBLFileHandle( const char* const pszPath, const char* pszMode )
    {
        OpenFileFromPath( pszPath, pszMode );
    }
    
    bool OpenFileFromPath( std::wstring& strPath, const char* pszMode )
    {
        char szPath[PATH_MAX * sizeof(strPath[0])];
        const wchar_t* wcs = strPath.c_str();
        wcsrtombs( szPath, &wcs, sizeof(szPath), NULL );
     
        OpenFileFromPath( szPath, pszMode );
    }
    
    bool OpenFileFromPath( const char* const pszPath, const char* pszMode )
    {
        CloseFile();
        
        m_pHandle = fopen( pszPath, pszMode );
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
    
    int GetFilePath( std::wstring& strPath, bool bResolveSymlinks ) const
    {
        assert( m_pHandle );
        int fd = GetDescriptor();
        strPath.resize( PATH_MAX );
        char szMbPath[PATH_MAX];
        int ret = fcntl( fd, F_GETPATH, szMbPath );
        if ( bResolveSymlinks )
        {
            char szTemp[PATH_MAX];
            strncpy( szTemp, szMbPath, strlen(szMbPath) );
            char* pszError = realpath( szTemp, szMbPath );
            assert( pszError );
        }
        mbstowcs( &strPath[0], szMbPath, strlen(szMbPath) );
        assert( strPath.length() == strlen(szMbPath) );
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

