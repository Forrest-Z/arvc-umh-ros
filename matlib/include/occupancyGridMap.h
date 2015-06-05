#pragma once
#ifndef __OCCUPANCY_GRID_MAP___
#define __OCCUPANCY_GRID_MAP___

#include "binMap.h"
#include "robotTypes.h"
#include "matFuns.h"
#include "rangeSensorData.h"
#include "gridMapInterface.h"

#define DILATERAD 2

/**
*
* Author: Miguel Julia <mjulia@umh.es> 
* 
* Date:   2008
* 
* Class occupancyGridMap
*
* This is an abstract class that implements some common parts of the grid mapping algorithms
*
*/
class occupancyGridMap : public gridMapInterface
{

///////////////////////////////////////////////////////////////////////
//-------------------------  Attributes  ------------------------------
///////////////////////////////////////////////////////////////////////

protected:

	unsigned short width;
	unsigned short height;
	float realWidth;
	float realHeight;
	float xorigin;		// Relation between discrete and real axis
	float yorigin;
	float resolution;

//---------------------------------------------------------------------	
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//-------------------------  Methods  --------------------------------
///////////////////////////////////////////////////////////////////////

public:

	/// Defaulf constructor
	occupancyGridMap();
	/// Constructor with size in meters and related to real coordinates
	occupancyGridMap(float w, float h, float res, float x, float y);
	/// Default destructor
	virtual ~occupancyGridMap();

	// These methods have to be implemented for each algorithm in derived classes:

	/// Initializes with size in meters and related to real coordinates
	virtual void initialize(float w, float h, float res, float x, float y)=0;
	/// clone method
	virtual occupancyGridMap* clone() const=0;
	/// assigment operator
	virtual gridMapInterface& operator=(const gridMapInterface&);

	/// return the occupancy probability of the cell
	virtual float getValue(const int& x, const int& y) const=0;
	/// return the occupancy probability of the cell
	virtual float getValue(const uint& x, const uint& y) const=0;
	/// return the occupancy probability of the cell
	virtual float getValueNotSafe(const int& x, const int& y) const=0;
	/// return the occupancy probability of the cell
	virtual float getValueNotSafe(const uint& x, const uint& y) const=0;
	/// return the occupancy probability of the cell
	float getValue(const uint& x, const uint& y, const uint& range) const;
	/// Resets the occupation probability for the total map
	virtual void reset()=0;
	/// Updates the occupancy grid using the new data
	virtual void update(const rangeSensorData& rsData, const pose& rpos, float disp)=0;
	
	/// sets the x real coordinates of pixel(0,0)
	void setXOrigin(float xori);
	/// sets the y real coordinates of pixel(0,0)
	void setYOrigin(float yori);
	/// sets the grid map resolution in meters
	void setResolution(float res);
	
	/// returns the width of the grid map
	int getWidth() const;
	/// returns the height of the grid map
	int getHeight() const;
	/// returns the width of the grid map
	float getRealWidth() const;
	/// returns the height of the grid map
	float getRealHeight() const;
	/// returns the x real coordinates of pixel(0,0)
	float getXOrigin() const;
	/// returns the y real coordinates of pixel(0,0)
	float getYOrigin() const;
	/// returns the grid map resolution in meters
	float getResolution() const;
	
	/// Returns a binMap of the frontiers
	int frontiers(binMap& frontiers) const;
	/// Returns a binMap of the frontiers inside the given zone
	int frontiersIn(const binMap& zone, binMap& frontiers) const;
	/// Evaluates the expected safe zone viewed from a given point
	void esz(int x, int y, binMap& esz, int dilaterad, int size) const;
	/// Evaluates the safe zone viewed from a given point
	void sz(int x, int y, binMap& esz, int dilaterad, int size) const;

	void fov(int x, int y, binMap& map, int dilaterad, int max) const;
	void rec_cast_light(binMap& map, int cx, int cy, int row, float start, float end, int radius, int xx, int xy, int yx, int yy, int id, int dilaterad) const;
	void secview(int x, int y, binMap& map, int dilaterad, int max) const;
	void rec_cast_light_safe(binMap& map, const int& cx, const int& cy, const int& row, float& start, const float& end, const int& radius, const int& xx, const int& xy, const int& yx, const int& yy, const int& dilaterad) const;

	/// Looks for gateways in a given zone
	void gateways(const binMap& esz, binMap& gateways) const;
	void clearUnconnect(binMap& vz, uint x, uint y) const;
	/// Return the occupied cells
	void occupiedCells(binMap& occupied, int dilateRad = 1) const;
	
	/// true if the cell is free
	bool isfree(const int& x, const int& y) const;
	/// true if the cell is occupied
	bool isoccupied(const int& x, const int& y) const;
	/// true if the cell is unknown
	bool isunknown(const int& x, const int& y) const;
	/// true if the cell is frontier
	bool isfrontier(const int& x, const int& y) const;

	/// true if the cell is free
	bool isfree(const uint& x, const uint& y) const;
	bool isfreeNotSafe(const uint& x, const uint& y) const;
	/// true if the cell is occupied
	bool isoccupied(const uint& x, const uint& y) const;
	/// true if the cell is unknown
	bool isunknown(const uint& x, const uint& y) const;
	/// true if the cell is frontier
	bool isfrontier(const uint& x, const uint& y) const;

	
	/// true if all cells in a given square mask of radius rad are free (Totally free)
	bool isfree(const uint& x, const uint& y, const uint& rad) const;	
	bool isfreeNotSafe(const uint& x, const uint& y, const uint& rad) const;	
	/// true if all cells in a diamond 1px mask of radius rad are free (Totally free)
	bool isfreeD(const uint& x, const uint& y) const;	
	/// true if a cell in a given square mask of radius rad is occupied (Partially occupied)
	bool isoccupied(const uint& x, const uint& y, const uint& rad) const;
	/// true if a cell in a given square mask of radius rad is unknown (Partially unknown)
	bool isunknown(const uint& x, const uint& y, const uint& size) const;
	
	/// Save function
	void saveMapAsImage(const char* file) const;
	/// Get the map as a new opencv image
	IplImage* getMapAsImage() const;
	/// Show binMap
	void showMap(const char* windowname)const;

	/// save a map to disk
	virtual int saveMapToFile(const char* file) const = 0;
	/// load a map from disk
	virtual int loadMapFromFile(const char* file) = 0;

	int countAccessible(const point* p, int numpoints, binMap& accessible) const;

	point toCell(float x, float y) const;
	pointf toCoords(int x, int y) const;

protected:

	int floodfill(const int& x, const int& y, binMap& processed) const;


//---------------------------------------------------------------------	
///////////////////////////////////////////////////////////////////////
};

inline void occupancyGridMap::setXOrigin(float xori)				{xorigin = xori;}
inline void occupancyGridMap::setYOrigin(float yori)				{yorigin = yori;}
inline void occupancyGridMap::setResolution(float res)				{resolution = res;}
inline int occupancyGridMap::getWidth() const					{return width;}
inline int occupancyGridMap::getHeight() const					{return height;}
inline float occupancyGridMap::getRealHeight() const				{return realHeight;}
inline float occupancyGridMap::getRealWidth() const				{return realWidth;}
inline float occupancyGridMap::getXOrigin() const				{return xorigin;}
inline float occupancyGridMap::getYOrigin() const				{return yorigin;}
inline float occupancyGridMap::getResolution() const				{return resolution;}
inline bool occupancyGridMap::isfree(const int& x, const int& y) const		{return (getValue(x,y) < 49.0f)? true: false;}
inline bool occupancyGridMap::isoccupied(const int& x, const int& y) const	{return (getValue(x,y) > 51.0f)? true: false;}
inline bool occupancyGridMap::isunknown(const int& x, const int& y) const	{float val = getValue(x,y);
									 	return (val >= 49.0f && val <= 51.0f)? true: false;}
inline bool occupancyGridMap::isfrontier(const int& x, const int& y) const	{return (isfree(x,y) && isunknown(x,y,1) && !isoccupied(x,y,1))? true : false;}
inline bool occupancyGridMap::isfree(const uint& x, const uint& y) const	{return (getValue(x,y) < 49.0f)? true: false;}
inline bool occupancyGridMap::isfreeNotSafe(const uint& x, const uint& y) const	{return (getValueNotSafe(x,y) < 49.0f)? true: false;}
inline bool occupancyGridMap::isoccupied(const uint& x, const uint& y) const	{return (getValue(x,y) > 51.0f)? true: false;}
inline bool occupancyGridMap::isunknown(const uint& x, const uint& y) const	{float val = getValue(x,y);
									 	return (val >= 49.0f && val <= 51.0f)? true: false;}
inline bool occupancyGridMap::isfrontier(const uint& x, const uint& y) const	{return (isfree(x,y) && isunknown(x,y,1) && !isoccupied(x,y,1))? true : false;}
inline point occupancyGridMap::toCell(float x, float y) const			{return point(	(int)floor((x-xorigin)/resolution),
												(int)floor((y-yorigin)/resolution));}
inline pointf occupancyGridMap::toCoords(int x, int y) const			{return pointf(	(x+0.5f)*resolution+xorigin,
												(y+0.5f)*resolution+yorigin);};

#endif
