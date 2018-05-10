from Hooklog3 import Hooklog3
import os

APIParRet = {
    # LIB
    "LoadLibrary": [["lpFileName"], ["Return","result"]],
    # PROC
    #"CreateProcess": [[], ["Return","result"]], # MIKE: this one has been removed by someone
    "CreateProcessInternal": [["lpApplicationName"], ["Return","result"]],
    "OpenProcess": [[], ["Return","result"]], # it has procName, but might be empty
    "ExitProcess": [[], ["Return","result"]],
    "WinExec": [["CmdLine"], ["Return","result"]],
    "CloseHandle": [[], ["Return","result"]],
    "CreateRemoteThread": [[], ["Return","result"]], # it has procName, but might be empty
    "TerminateProcess": [[], ["Return","result"]],
    "TerminateThread": [["hThread"], ["Return","result"]],
    "CreateThread": [["dwThreadId"], ["Return","result"]],
    "OpenThread": [["dwThreadId"], ["Return","result"]],
    # FILE
    "CopyFile": [["lpExistingFileName"], ["Return","result"]], # include lpNewFileName?
    "CreateFile": [["hName", "desiredAccess"], ["Return","result"]], #MIKE: actually it is 'lpFileName' in spec.
    "WriteFile": [[], ["Return","result"]],
    "ReadFile": [[], ["Return","result"]],
    "DeleteFile": [["fileName"], ["Return","result"]], #MIKE: actually it is 'lpFileName' in spec.
    # REG
    "RegOpenCurrentUser": [[], ["Return","result"]],
    "RegQueryValue": [["hKey"], ["Return","result"]],
    "RegEnumValue": [["hKey"], ["Return","result"]],
    "RegOpenKey": [["hKey"], ["Return","result"]],
    "RegCloseKey": [["hKey"], ["Return","result"]],
    "RegSetValue": [["hKey"], ["Return","result"]],
    "RegCreateKey": [["hKey"], ["Return","result"]],
    "=RegDeleteKey": [["hKey"], ["Return","result"]], # MIKE: misprint the '=' #============
    "RegDeleteKey": [["hKey"], ["Return","result"]], # MIKE: misprint the '='
    "RegDeleteValue": [["hKey"], ["Return","result"]],
    # NET winhttp.dll
    "WinHttpConnect": [["pswzServerName"], ["Return","result"]],
    "WinHttpCreateUrl": [["wszUrl"], ["Return","result"]],
    "WinHttpOpen": [["pwszUserAgent"], ["Return","result"]],
    "WinHttpOpenRequest": [["pwszObjectName"], ["Return","result"]],
    "WinHttpReadData": [["lpBuffer"], ["Return","result"]],
    "WinHttpSendRequest": [["pwszHeaders"], ["Return","result"]],
    "WinHttpWriteData": [["lpBuffer"], ["Return","result"]],
    "WinHttpGetProxyForUrl": [["lpcwszUrl"], ["Return","result"]],
    # NET winnet32.dll
    "InternetOpen": [["lpszAgent"], ["Return","result"]],
    "InternetConnect": [["lpszServerName"], ["Return","result"]],
    "HttpSendRequest": [["lpszHeaders"], ["Return","result"]],
    "GetUrlCacheEntryInfo": [["lpszUrlName"], ["Return","result"]],
}

API_list = APIParRet.keys()

# -1:keep, 0:*, 1:1stLayer,10:negative, 20:postivite, 30:include
subPathDict = {
    "control panel": {'desktop': (-1,'desktop')},
    "system": {
        'setup': (-1,'sys_setup'),
        'controlset001':{'control':{'class': (0,'sys_ctlSet001_ctl_class\\*')}},
        'currentcontrolset':{
            'control':{
                'session manager': (0,'sys_curCtlSet_ctl_sessionManager\\*'),
                'mediaproperties': (0,'sys_curCtlSet_ctl_mediaProperties\\*'),
                'productoptions': (0,'sys_curCtlSet_ctl_productoptions\\*')},
            'services':{
                'netbt': (0,'sys_curCtlSet_svc_netbt\\*'),
                'tcpip': (10,'sys_curCtlSet_svc_tcpip', ['max','min','timeout']),
                'dnscache': (10,'sys_curCtlSet_svc_dnscache', ['max','min','timeout']),
                'ldap': (10,'sys_curCtlSet_svc_ldap', ['max','min','timeout']),
                'winsock': (10,'sys_curCtlSet_winsock', ['max','min','timeout']),
                'winsock2':{
                    'parameters':{
                        'protocol_catalog9': (0,'sys_curCtlSet_svc_winsock2_catalog9\\*'),
                        'namespace_catalog5': (0,'sys_curCtlSet_svc_winsock2_catalog5\\*'),
                        'ELSE' : (10,'sys_curCtlSet_svc_winsock2', ['max','min','timeout'])}, #
                    'ELSE' : (10,'sys_curCtlSet_svc_winsock2', ['max','min','timeout'])}, #
                'windows test 5.0': (1,'sys_curCtlSet_svc_winTest5.0')}},
        'ELSE':(-1,'sys')
    },
    'software':{
        'microsoft':{
            'commandprocessor': (0,'soft_ms_commandprocessor\\*'),
            'audiocompressionmanager': (0,'soft_ms_audiocompressionmanager\\*'),
            'multimedia':{
                'audio compression manager': (0,'soft_ms_mulitimedia_audiocompressionmanager\\*'),
                'ELSE' :(20,'soft_ms_multimedia',['audio'])},
            'ole': (0,'soft_ms_ole\\*'),
            'ctf': (0,'soft_ms_ctf\\*'),
            'com3': (0,'soft_ms_com3\\*'),
            'tracing': (1,'soft_ms_tracing'),
            'internet explorer':{'main':{'featurecontrol':(-1,'soft_ms_IE_featureCtl')}},
            'rpc': (10,'soft_ms_rpc', ['max','min','timeout']),
            'wbem':{
                'cimom': (20,'soft_ms_wbem_cimom', ['log']),
                'wmic': (0,'soft_ms_wbem_wmic\\*')},
            'windows nt':{'currentversion':{
                    'languagepack':(0,'soft_ms_winNT_languagepack\\*'),
                    'winlogon':(0,'soft_ms_winNT_winlogon\\*'),
                    'fontlink':(0,'soft_ms_winNT_fontlink\\*'),
                    'fontsubstitutes':(0,'soft_ms_winNT_fontsubstitutes\\*'),
                    'imm':(0,'soft_ms_winNT_imm\\*'),
                    'csdversion':(0,'soft_ms_winNT_csdversion\\*')}},
            'windows':{
                'currentversion':{
                    'thememanager':(0,'soft_ms_win_thememanager\\*'),
                    'telephony':(0,'soft_ms_win_telephony\\*'),
                    'profilelist':(0,'soft_ms_win_profilelist\\*'),
                    'programfilesdir':(0,'soft_ms_win_programfilesdir\\*'),
                    'commonfilesdir':(0,'soft_ms_win_commonfilesdir\\*'),
                    'explorer':{
                        'user shell folders': (0,'soft_ms_win_explorer_userShellFolder\\*'),
                        'shell folders': (0,'soft_ms_win_explorer_shellFolders\\*'),
                        'ELSE' : (-1,'soft_ms_win_explorer')},
                    'internet settings':{
                        'zones':(0,'soft_ms_win_internetSettings_zones\\*'),
                        'cache':{'paths':(0,'soft_ms_win_internetSettings_cache_paths\\*')},
                        'ELSE':(10,'soft_ms_win_internetSettings', ['max','min','timeout','length','bound','limit','timesecs','range'])},
                    'ELSE':(-1,'soft_ms_win_currentversion')
                }}
        }
    },
    'clsid':{'{REG}':{
            'inprocserver32':(0,'clsid_{REG}_inprocserver32\\*'),
            'appid':(0,'clsid_{REG}_appid\\*')
        }},
    'appid':{'{REG}':(0,'appid_{REG}\\*')},
    'interface':{'{REG}':{'proxystubclsid32':(0,'interface_{REG}_proxystubclsid32\\*')}},
    '{REG}':{
        'environment':(-1,'{REG}_envr'),
        'volatile environment':(-1,'{REG}_volatileEnvr')
    }
}

# -1:keep, 0:*, 1:1stLayer,10:negative, 20:postivite,
def subPathstrim(subpath): # -1, 0, 10, 20
    subFlag = False
    startIndex = 0
    startToken = ''
    returnVal = ''
    
    tokens = subpath.split('\\')
    for i, token in enumerate(tokens):
        if token in subPathDict:
            startIndex = i
            startToken = token
            break
        else:
            return ('\\').join(tokens)

    endIndex = startIndex
    endToken = startToken
    tmpDict = subPathDict
    endFlag = True if type(tmpDict[endToken])==tuple else False
    
    while( not endFlag ): # not to the end 
        tmpDict = tmpDict[endToken]
        endIndex += 1
        endToken = tokens[endIndex]
        if endToken in tmpDict:
            endFlag = True if type(tmpDict[endToken])==tuple else False
        else:
            endToken = 'ELSE'
            endIndex -= 1
            break
    
    #print subPath ####
    subType = tmpDict[endToken][0]
    subString = tmpDict[endToken][1]
    keepFrontString = ('\\').join(tokens[:startIndex])
    
    if subType == 0:
        returnVal = keepFrontString + subString
    elif subType == -1:
        keepEndtString = ('\\').join(tokens[endIndex+1:])
        returnVal = keepFrontString + subString + '\\' + keepEndtString
    elif subType == 1:
        keepEndtString = ('\\').join(tokens[endIndex+1:endIndex+2])
        returnVal = keepFrontString + subString + '\\' + keepEndtString
    elif subType == 10:
        ignoreli = tmpDict[endToken][2]
        keepEndtString = ('\\').join(tokens[endIndex+1:])
        keepFlag = True
        for k in ignoreli:
            if k in keepEndtString:
                returnVal = keepFrontString + subString + '\\*'
                keepFlag = False
                break
        if keepFlag:
            returnVal = keepFrontString + subString + '\\' + keepEndtString
    elif subType == 20:
        keepli = tmpDict[endToken][2]
        keepEndtString = ('\\').join(tokens[endIndex+1:])
        ignoreFlag = True
        for k in keepli:
            if k in keepEndtString:
                returnVal = keepFrontString + subString + '\\'+ k
                ignoreFlag = False
                break
        if ignoreFlag:
            returnVal = keepFrontString + subString + '\\*' 
    
    return returnVal

#MIKE: 20170713
def inKey(this_dict, myKey):
    return next((key for key in this_dict if key in myKey), False) # MIKE: key in mykey is correct!!


### MIKE: 20170714 new
def replace_strings(key):
    tokens = key.split('\\')

    rvalue = ""
    for token in tokens:
        
        # SID like token
        if token.count('-') > 3:
            rvalue += "\\{REG}"
        # file like token
        elif len(token.split('.')) >= 2 and token.split('.')[-1] in ["exe", "txt", "bat", "clb", "dll"] : #MIKE: 20170714, hack
            rvalue += "\\{FIL}." + token.split('.')[-1]
        # mist
        elif "mshist" in token:
            rvalue += "\\{MSHISTDATE}"
        else:
            rvalue += ('\\' + token)
            
    # MIKE: 20170714, there was a bug in old __remove_fileName(), it removes the first \\ accidentally
    # so I remove the first \\ here (so that subPathstrim() could work correctly).
    if rvalue.startswith('\\'): rvalue = rvalue[1:]
    if rvalue.startswith('\\'): rvalue = rvalue[1:]
            
    # sessionID
    if "software\\microsoft\\windows\\currentversion\\explorer\\sessioninfo" in rvalue:
        rvalue = rvalue[:rvalue.rindex('\\')] + "SESSIONID"

    return rvalue
###


def libTrans(value):
    global dir_dict
    if value == "": return "NON@NON@NON" # DIR@LIB@EXT
    
    DIR = LIB = EXT = "NON" # MIKE: 20170713, use capital
    
    lvalue = value.lower().replace('/', '\\')
    tokens = lvalue.split('\\')
    
    # DIR, MIKE: 20170714 change logic
    #if lvalue[1] != ':': DIR = "SYS"
    #elif lvalue[0] == '\\': DIR = "LOC" # MIKE: 20170713, really? != ??
    if len(tokens) == 1:
        DIR = "SYS"
    else:
        key = inKey(dir_dict, lvalue)
        DIR = dir_dict[key] if key else "ARB"
    
    # LIB
    LIB = tokens[-1].split('.')[0]
    
    # EXT
    t = tokens[-1].split('.')
    ext = t[-1]
    if ext == "" or len(t) == 1: # extension is . or no extension
        EXT = "DLL"
    else:
        EXT = ext.upper()
    
    return DIR+"@"+LIB+"@"+EXT


def execTrans(value):
    return fileTrans(value)

def cmdTrans(value):
    return value

def thdTrans(value):
    return "NON"

def fileTrans(value):
    global dir_dict
    if value == "": return "NON@NON"
    
    DIR = EXT = "NON" # MIKE: 20170713, use capital
    
    lvalue = value.lower().replace('/', '\\')
    tokens = lvalue.split('\\')
    
    # DIR, MIKE: 20170714 change logic
    if lvalue[:4] == ("\\\\.\\"):
        DIR = lvalue
    elif lvalue == 'conin$' or lvalue == 'conout$':
        DIR = 'CONSOLE'
    elif len(tokens) == 1:
        DIR = "LOC"
    else:
        key = inKey(dir_dict, lvalue)
        DIR = dir_dict[key] if key != False else "ARB"
    
    # EXT
    t = tokens[-1].split('.')
    ext = t[-1]
    if ext == "" or len(t) == 1: # extension is . or no extension
        EXT = "NON"
    else:
        EXT = ext.upper()
    
    return DIR+"@"+EXT

def fileDesiredAcc(value):
    return '&&dA:'+ value.replace(' ', ';')

def keySupPathTrans(value):
    keyBasic = keyTrans(value)
    HK = keyBasic[:keyBasic.index("@")]
    KEY = keyBasic[keyBasic.index("@")+1:]
    
    try:
        KEY = subPathstrim(KEY)
    except:
        KEY = KEY 
    return HK+"@"+KEY

def keyTrans(value):
    global hkey_dict
    if value == "": return "NON@NON"
    
    HK = KEY = "NON"
    
    lvalue = value.lower()
    tokens = lvalue.split('\\')
    
    # HK
    hkey = inKey(hkey_dict, tokens[0])
    HK = hkey_dict[hkey] if hkey else "SUBK"

    # KEY
    KEY = lvalue[lvalue.find('\\'):]
    KEY = replace_strings(KEY) # MIKE: 20170714, combine all rules

    return HK+"@"+KEY

funcDict = {
    "LoadLibrarylpFileName": libTrans,
    "CreateProcessInternallpApplicationName": execTrans,
    "WinExecCmdLine": cmdTrans,
    "CreateThreaddwThreadId": thdTrans, 
    "CreateFilehName": fileTrans,
    "CreateFiledesiredAccess": fileDesiredAcc, # NEW 
    "CopyFilelpExistingFileName": fileTrans,
    "DeleteFilefileName": fileTrans,
    "RegQueryValuehKey": keySupPathTrans,
    "RegEnumValuehKey": keySupPathTrans,
    "RegOpenKeyhKey": keySupPathTrans,
    "RegCloseKeyhKey": keySupPathTrans,
    "RegSetValuehKey": keySupPathTrans,
    "RegCreateKeyhKey": keyTrans,
    "=RegDeleteKeyhKey": keyTrans, #####
    "RegDeleteKeyhKey": keyTrans,
    "RegDeleteValuehKey": keyTrans
}

dir_dict = {
    "\\windows\\system32\\": "SYS",
    "\\windows\\system\\": "SYS",
    "\\program files\\": "PRO",
    #"\\windows\\": "WIN",
    #"\\documents and settings\\all users\\": "USR",
    "\\documents and settings\\": "USR",
    "\\docume~1\\": "USR",
    "\\windows\\temp\\": "TMP",
    ":\\temp\\": "TMP"
}

hkey_dict = {
    "hkey_classes_root": "HKCR",
    "hkey_current_user": "HKCU",
    "hkey_local_machine": "HKLM",
    "hkey_users": "HKUS",
    "hkey_current_config": "HKCC"
}

class FeatureHooklog3(Hooklog3):
    
    # MIKE: by default: par = 1
    def __init__(self, path, in_parseFirstPar = 1):
        self.path = path
        self.par = True # overwrite it
        self.li = list()
        self.length = 0
        
        self._parseDigitname()
        self._parseHooklog() # use Hooklog3's _parseHooklog
    
    def __str__(self):
        return "class FeatureHooklog3, %s, len = %d, digit name = %s" % (self.path, self.length, self.digitname)
    
    def _skipAPI(self, api): # MIKE: 20170708, skip some apis
        # FeatureHooklog3 does skip API
        return True if api not in API_list else False
    
    # private functions
    def __parTrans(self, api, key, value, trans):
        if not trans: return value
        global funcDict
        return funcDict[api+key](value) if api+key in funcDict else value

    # protected functions
    # MIKE: 20160307, in FeatureHooklog
    def _getParValue(self, api, handle):
        parword = ""
        retword = ""
        
        while 1:
            pos = handle.tell()
            line = handle.readline().decode('ISO 8859-1').strip() # MIKE: 20170616, for python 3
            if not line: # reach to the end of file
                break
            if line[0] == '#': # reach to next call
                handle.seek(pos)
                break
        
            delimiter = line[1:].find('=') +1 # MIKE: for =Reg
            p = line[:delimiter]
            v = line[delimiter+1:].strip() #===value
        
            if p in APIParRet[api][0]: # parameter
                parword += self.__parTrans(api, p, v, 1) # note: v could be empty!
            elif p in APIParRet[api][1]: # return
                #retword += v\n",
                # MIKE: 20170811/14, winnowing the ret globally and be compatible with old hooklog\n",
                if v[0] == "S":
                    retword += "P" # positive
                elif v[0] == "F":
                    retword += "N" # negative
                elif v == "0":
                    retword += "0" # zero
                else:
                    value = int(v, 16)
                    if value > 0:
                        retword += "P"
                    elif value < 0:
                        retword += "N"
                    else:
                        retword += "0"
            else: # fail safe, should not happen if APIParRet is correct!
                #if p in ['Return', 'result']:
                #    retword += v
                #else:
                #    parword += (line + '?')
                pass
                    
        return api + "#PR#" + parword + "#PR#" + retword
    
    def getHkli_containTS(self): # return with Timestamp
        return self.li
    
    def getHkli_noContainTS(self): # return without Timestamp
        hookli_noTS = list()
        for (timestamp, api) in self.li:
            hookli_noTS.append(api)
        
        return hookli_noTS