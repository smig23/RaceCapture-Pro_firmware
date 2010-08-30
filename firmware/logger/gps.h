#ifndef GPS_H_
#define GPS_H_

void startGPSTask();

void GPSTask( void *pvParameters );

void getUTCTimeFormatted(char * buf);
float getUTCTime();

double getLatitude();
double getLongitude();

int getGPSQuality();
int getSatellitesUsedForPosition();
float getGPSVelocity();
char * getGPSDataLine();


void parseGGA(char *data);
void parseVTG(char *data);
void parseGSA(char *data);
void parseGLL(char *data);
void parseZDA(char *data);
void parseGSV(char *data);
void parseRMC(char *data);
			
#endif /*GPS_H_*/