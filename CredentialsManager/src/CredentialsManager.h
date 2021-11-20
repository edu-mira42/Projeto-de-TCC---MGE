#ifndef Credentials_h
#define Credentials_h

#include "Arduino.h"

class CredentialsManager {

    public:

        CredentialsManager();
        void begin(int baudRate = 115200);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        String readFile(const char * fileName);
        int readIntValue(const char* fileName, String content = "");

        String readFileSD(const char * fileName);
        int readIntValueSD(const char* fileName, String content = "");

        int getSensorOP(String content = "");
        const char* getSSID(String ssid = "");
        const char* getPASS(String pass = "");
        const char* getSSIDAP(String ssidAP = "");
        const char* getPASSAP(String passAP = "");
        const char* getUserHTTP(String userHTTP = "");
        const char* getPassHTTP(String passHTTP = "");
        int getOPmode();
        int getAPmode();

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////

        void format();
        void writeFile(const char * fileName, const char * message);
        void appendFile(const char * fileName, const char * message);
        void writeIntValue(const char* fileName, int value = 0);

        void writeFileSD(const char * fileName, const char * message);
        void appendFileSD(const char * fileName, const char * message);
        void writeIntValueSD(const char* fileName, int value = 0);

        void saveSensorOP(int value = 0);
        void saveSSID(const char* SSID);
        void savePASS(const char* PASS);
        void saveSSIDAP(const char* SSIDAP);
        void savePASSAP(const char* PASSAP);
        void saveUserHTTP(const char* USERHTTP);
        void savePassHTTP(const char* PASSHTTP);
        void saveOPmode(int opM);
        void saveAPmode(int apM);

    private:
};
#endif