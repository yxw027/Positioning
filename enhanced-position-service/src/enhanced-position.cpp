/**************************************************************************
* @licence app begin@
*
* SPDX-License-Identifier: MPL-2.0
*
* \ingroup EnhancedPositionService
* \author Marco Residori <marco.residori@xse.de>
*
* \copyright Copyright (C) 2014, XS Embedded GmbH
* 
* \license
* This Source Code Form is subject to the terms of the
* Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
* this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* @licence end@
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "enhanced-position.h"
#include "positioning-constants.h"
#include "log.h"

DLT_IMPORT_CONTEXT(gCtx);

EnhancedPosition* EnhancedPosition::mpSelf = 0;

static DBus::Variant variant_double(double d)
{
  DBus::Variant variant;
  DBus::MessageIter iter=variant.writer();
  iter << d;
  return variant;
}

static DBus::Variant variant_uint16(uint16_t i)
{
  DBus::Variant variant;
  DBus::MessageIter iter=variant.writer();
  iter << i;
  return variant;
}

static DBus::Variant variant_int32(int32_t i)
{
  DBus::Variant variant;
  DBus::MessageIter iter=variant.writer();
  iter << i;
  return variant;
}

static DBus::Variant variant_string(std::string s)
{
  DBus::Variant variant;
  DBus::MessageIter iter=variant.writer();
  iter << s;
  return variant;
}

EnhancedPosition::EnhancedPosition(DBus::Connection & connection, const char * path)
  : DBus::ObjectAdaptor(connection, path)
{
    mpSelf = this;
}

EnhancedPosition::~EnhancedPosition()
{
    mpSelf = 0;
}

::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > EnhancedPosition::GetVersion()
{
  ::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > Version;

  Version._1 = 3;
  Version._2 = 0;
  Version._3 = 0;
  Version._4 = std::string("05-08-2014");

  return Version;
}
    
std::map< uint16_t, ::DBus::Variant > EnhancedPosition::GetPositionInfo(const std::vector< uint16_t >& valuesToReturn)
{
  std::map< uint16_t, ::DBus::Variant > Data;

  TGNSSPosition position;

  bool isPosRequested = false;
  bool isCourseRequested = false;

  for (int i = 0; i < valuesToReturn.size(); i++)
  {
    if ((valuesToReturn[i] == POS_LATITUDE) || 
        (valuesToReturn[i] == POS_LONGITUDE) || 
        (valuesToReturn[i] == POS_ALTITUDE))
    {
      isPosRequested = true;
    }

    if ((valuesToReturn[i] == POS_HEADING) || 
        (valuesToReturn[i] == POS_SPEED) || 
        (valuesToReturn[i] == POS_CLIMB ))
    {
      isCourseRequested = true;
    }
  }

  if(isPosRequested)
  {
    if(gnssGetPosition(&position))
    {
      if (position.validityBits & GNSS_POSITION_LATITUDE_VALID)
      {
        Data[POS_LATITUDE] = variant_double(position.latitude);
      }

      if (position.validityBits & GNSS_POSITION_LONGITUDE_VALID)
      {
        Data[POS_LONGITUDE] = variant_double(position.longitude);
      }

      if (position.validityBits & GNSS_POSITION_ALTITUDEMSL_VALID)
      {
        Data[POS_ALTITUDE] = variant_double(position.altitudeMSL);
      }
    }
  }

  if(isCourseRequested)
  {
    if(gnssGetPosition(&position))
    {
      if (position.validityBits & GNSS_POSITION_HEADING_VALID)
      {
        Data[POS_HEADING] = variant_double(position.heading);
      }

      if (position.validityBits & GNSS_POSITION_HSPEED_VALID)
      {
        Data[POS_SPEED] = variant_double(position.hSpeed);
      }

      if (position.validityBits & GNSS_POSITION_VSPEED_VALID)
      {
        Data[POS_CLIMB] = variant_double(position.vSpeed);
      }
    }
  }

  return Data;
}   

std::map< uint16_t, ::DBus::Variant > EnhancedPosition::GetSatelliteInfo()
{
  std::map< uint16_t, ::DBus::Variant > SatelliteInfo;

  throw DBus::ErrorNotSupported("Method not supported yet");

  return SatelliteInfo;
}

std::map< uint16_t, ::DBus::Variant > EnhancedPosition::GetTime()
{
  std::map< uint16_t, ::DBus::Variant > Time;

  throw DBus::ErrorNotSupported("Method not supported yet");

  return Time;
}

void EnhancedPosition::sigPositionUpdate(const TGNSSPosition position[], uint16_t numElements)
{
  bool latChanged = false;
  bool lonChanged = false;
  bool altChanged = false;
  bool speedChanged = false;
  bool headingChanged = false;
  bool climbChanged = false;


  if (position == NULL || numElements < 1)
  {
    LOG_ERROR_MSG(gCtx,"sigPositionUpdate failed!");
    return;
  }

  for (int i = 0; i< numElements; i++)
  {
    LOG_INFO(gCtx,"Position Update[%d/%d]: lat=%f lon=%f alt=%f",
             i+1,
             numElements,
             position[i].latitude, 
             position[i].longitude,
             position[i].altitudeMSL);
        
    if (latChanged == false)
    {
      latChanged = (position[i].validityBits & GNSS_POSITION_LATITUDE_VALID);
    }

    if (lonChanged == false)
    {
      lonChanged = (position[i].validityBits & GNSS_POSITION_LONGITUDE_VALID);
    }

    if (altChanged == false)
    {
      altChanged = (position[i].validityBits & GNSS_POSITION_ALTITUDEMSL_VALID);
    }

    if (speedChanged == false)
    {
      speedChanged = (position[i].validityBits & GNSS_POSITION_HSPEED_VALID);
    }

    if (headingChanged == false)
    {
      headingChanged = (position[i].validityBits & GNSS_POSITION_HEADING_VALID);
    }

    if (climbChanged == false)
    {
      climbChanged = (position[i].validityBits & GNSS_POSITION_VSPEED_VALID);
    }

  }

  //in a real product, the coordinates would be used for dead-reckoning.
  //in this proof of concept, the client application is simply notified 
  //about changes of latitude, longitude and/or altitude
  std::vector< uint16_t > changedValues;
  std::vector<uint16_t>::iterator it;
  it = changedValues.begin();

  if (latChanged)
  {
     it = changedValues.insert(it,POS_LATITUDE);
  }

  if (lonChanged)
  {
     it = changedValues.insert(it,POS_LONGITUDE);
  }

  if (altChanged)
  {
     it = changedValues.insert(it,POS_ALTITUDE);
  }

  if (speedChanged)
  {
     it = changedValues.insert(it,POS_SPEED);
  }

  if (headingChanged)
  {
     it = changedValues.insert(it,POS_HEADING);
  }

  if (climbChanged)
  {
     it = changedValues.insert(it,POS_CLIMB);
  }

  if (!mpSelf)
  {
    LOG_ERROR_MSG(gCtx,"Null pointer!");    
    return;
  }
    
  //notify clients
  mpSelf->PositionUpdate(changedValues);


}

void EnhancedPosition::sigGNSSAccuracyUpdate(const TGNSSPosition position[], uint16_t numElements)
{ 
  bool pdopChanged = false;
  bool hdopChanged = false;
  bool vdopChanged = false;
  bool usatChanged = false; //used satellites

  if (position == NULL || numElements < 1)
  {
      LOG_ERROR_MSG(gCtx,"sigAccuracyUpdate failed!");
      return;
  }

  for (int i = 0; i< numElements; i++)
  {
      LOG_INFO(gCtx,"Accuracy Update[%d/%d]: pdop=%f hdop=%f vdop=%f \
               sigmaHPosition=%f sigmaAltitude=%f",
               i+1,
               numElements,
               position[i].pdop, 
               position[i].hdop,
               position[i].vdop,
               position[i].sigmaHPosition,
               position[i].sigmaAltitude);

    if (pdopChanged == false)
    {
      pdopChanged = (position[i].validityBits & GNSS_POSITION_PDOP_VALID);
    }

    if (hdopChanged == false)
    {
      hdopChanged = (position[i].validityBits & GNSS_POSITION_HDOP_VALID);
    }

    if (vdopChanged == false)
    {
      vdopChanged = (position[i].validityBits & GNSS_POSITION_VDOP_VALID);
    }

    if (usatChanged == false)
    {
      usatChanged = (position[i].validityBits & GNSS_POSITION_USAT_VALID);
    }
  }

  //in a real product, the accuracy would be used for dead-reckoning.
  //in this proof of concept, the client application is simply notified 
  //about accuracy changes
  std::vector<uint16_t>::iterator it;
  
  std::vector< uint16_t > changedAccuracyValues;
  it = changedAccuracyValues.begin();

  if (pdopChanged)
  {
    it = changedAccuracyValues.insert(it,POS_PDOP);
  }

  if (hdopChanged)
  {
    it = changedAccuracyValues.insert(it,POS_HDOP);
  }
  
  if (vdopChanged)
  {
    it = changedAccuracyValues.insert(it,POS_VDOP);
  }  

  if (usatChanged)
  {
    it = changedAccuracyValues.insert(it,POS_USED_SATELLITES);
  }

  //todo: handle other field-changes here (accuracy and status)

  if (!mpSelf)
  {
    LOG_ERROR_MSG(gCtx,"Null pointer!");    
    return;
  }
  
  mpSelf->GNSSAccuracyUpdate(changedAccuracyValues);
}

void EnhancedPosition::sigStatusUpdate(const TGNSSPosition position[], uint16_t numElements)
{ 

  bool fixStatusChanged = false;
  bool fixTypeBitsChanged = false;

  if (position == NULL || numElements < 1)
  {
      LOG_ERROR_MSG(gCtx,"sigStatusUpdate failed!");
      return;
  }

  for (int i = 0; i< numElements; i++)
  {
      LOG_INFO(gCtx,"Status Update[%d/%d]: fixStatus=%d fixTypeBits=0x%08X",
               i+1,
               numElements,
               position[i].fixStatus,
               position[i].fixTypeBits);
               
    if (fixStatusChanged == false)
    {
      fixStatusChanged = (position[i].validityBits & GNSS_POSITION_STAT_VALID);
    }

    if (fixTypeBitsChanged == false)
    {
      fixTypeBitsChanged = (position[i].validityBits & GNSS_POSITION_TYPE_VALID);
    }

  }

  //in a real product, the accuracy would be used for dead-reckoning.
  //in this proof of concept, the client application is simply notified 
  //about status changes
  std::vector<uint16_t>::iterator it;
  
  std::vector< uint16_t > changedStatusValues;
  it = changedStatusValues.begin();

  if (fixStatusChanged)
  {
    it = changedStatusValues.insert(it,POS_GNSS_FIX_STATUS);
  }

  if (!mpSelf)
  {
    LOG_ERROR_MSG(gCtx,"Null pointer!");    
    return;
  }
  
  mpSelf->StatusUpdate(changedStatusValues);
}


void EnhancedPosition::cbPosition(const TGNSSPosition position[], uint16_t numElements)
{
    sigPositionUpdate(position, numElements);
    sigGNSSAccuracyUpdate(position, numElements);
    sigStatusUpdate(position, numElements);
}


void EnhancedPosition::cbSatelliteDetail(const TGNSSSatelliteDetail satelliteDetail[], uint16_t numElements)
{
  if (satelliteDetail == NULL || numElements < 1)
  {
    LOG_ERROR_MSG(gCtx,"cbSatelliteDetail failed!");
    return;
  }

  for (int i = 0; i<numElements; i++)
  {
    LOG_INFO(gCtx,"SatelliteDetail Update[%d/%d]: satelliteId=%d azimuth=%d elevation=%d SNR=%d",
             i+1,
             numElements,
             satelliteDetail[i].satelliteId, 
             satelliteDetail[i].azimuth,
             satelliteDetail[i].elevation,                                   
             satelliteDetail[i].SNR);
  }  

  if (!mpSelf)
  {
    LOG_ERROR_MSG(gCtx,"Null pointer!");    
    return;
  }
    
  //todo: notify clients
  //mpSelf->SatelliteInfoUpdate(changedValues);
}

bool EnhancedPosition::checkMajorVersion(int expectedMajor)
{
  int major = -1;

  gnssGetVersion(&major, 0, 0);

  if (major != expectedMajor)
  {
    LOG_ERROR(gCtx,"Wrong API version: gnssGetVersion returned unexpected value %d != %d",
              major, 
              expectedMajor);

    return false;
  }

  return true;
}

void EnhancedPosition::run()
{
  if(!checkMajorVersion(3))
  {
    exit(EXIT_FAILURE);
  }

  if(!gnssInit())
  {
    exit(EXIT_FAILURE);
  }

  LOG_INFO_MSG(gCtx,"Starting EnhancedPosition dispatcher...");

  gnssRegisterPositionCallback(&cbPosition);
  gnssRegisterSatelliteDetailCallback(&cbSatelliteDetail);
}

void EnhancedPosition::shutdown()
{
  LOG_INFO_MSG(gCtx,"Shutting down EnhancedPosition dispatcher...");

  gnssDeregisterPositionCallback(&cbPosition);
  gnssDeregisterSatelliteDetailCallback(&cbSatelliteDetail);
  gnssDestroy();
}


