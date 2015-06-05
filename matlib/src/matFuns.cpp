#include <cv.h>
#include <string.h>
#include <iostream>
#include <time.h>
#include <ctime>    // For time()
#include <cstdlib>  // For srand() and rand()
#include <matFuns.h>


// mahalanobis distance
float mahalanobis(const Ematrix& x, const Ematrix& sigma){
	return sqrt((x.transpose()*(sigma.inverse()*x)).get(0,0));
}

float normal(const Ematrix& x, const Ematrix& sigma){
	return (1/(sqrt(pow(2*M_PI,sigma.getRows())*sigma.det() ))*exp(-0.5* (x.transpose()*(sigma.inverse()*x)).get(0,0) ) );
}

pos3d base2global(const pos3d& base, const pose& pos){
	float costh = (float)cos(pos.th);
	float sinth = (float)sin(pos.th);
	return pos3d(	costh * base.getX() - sinth * base.getY() + pos.x,
			sinth * base.getX() + costh * base.getY() + pos.y,
			base.getZ());
}

pos3d global2base(const pos3d& global, const pose& pos){
	float auxx = global.getX() - pos.x;
	float auxy = global.getY() - pos.y;
	float costh = (float)cos(pos.th);
	float sinth = (float)sin(pos.th);
	return pos3d(     costh*auxx + sinth*auxy,
			- sinth*auxx + costh*auxy,
			  global.getZ());
}

pose4 base2global(const pose4& base, const pose& pos){
	float costh = (float)cos(pos.th);
	float sinth = (float)sin(pos.th);
	float angle = base.th + pos.th;
	if 	(angle>PI)	angle-= PIx2;
	else if (angle <= -PI)	angle+= PIx2;

	return pose4(	costh * base.x - sinth * base.y + pos.x,
			sinth * base.x + costh * base.y + pos.y,
			base.z,
			angle);
}

pose4 global2base(const pose4& global, const pose& pos){
	float auxx = global.x - pos.x;
	float auxy = global.y - pos.y;
	float costh = (float)cos(pos.th);
	float sinth = (float)sin(pos.th);
	float angle = global.th - pos.th;
	if 	(angle>PI)	angle-= PIx2;
	else if (angle <= -PI)	angle+= PIx2;

	return pose4(     costh*auxx + sinth*auxy,
			- sinth*auxx + costh*auxy,
			  global.z,
			  angle);
}

posCil3d cart2cil(const pos3d& xyz){
	return posCil3d( sqrt(xyz.getX()*xyz.getX()+xyz.getY()*xyz.getY()),
					 atan2(xyz.getY(),xyz.getX()),
					 xyz.getZ());
}

float poly(const double* pol, const double x, int grad){
	float y=0;
	for(int i = 0; i <= grad; i++)	y += pol[i]*pow(x,grad-i);
	return(y);
}

double evalDisp(const Ematrix& covariance){

	CvMat* cov = cvCreateMat(2,2,CV_32FC1);
	CvMat* evects = cvCreateMat(2,2,CV_32FC1);
	CvMat* evals = cvCreateMat(2,1,CV_32FC1);
	cvmSet(cov,0,0, covariance(0,0));
	cvmSet(cov,0,1, covariance(0,1));
	cvmSet(cov,1,0, covariance(1,0));
	cvmSet(cov,1,1, covariance(1,1));

	cvEigenVV( cov, evects, evals, DBL_EPSILON);

	float a = 3*sqrt(cvmGet(evals,0,0));
	float b = 3*sqrt(cvmGet(evals,1,0));
	if (a != a) a = 0.0f;
	if (b != b) b = 0.0f;

	cvReleaseMat(&cov);
	cvReleaseMat(&evects);
	cvReleaseMat(&evals);

	return PI*a*b;
}

/// this function implements a random number with gaussian probability
/// the genaration uses the box-muller transform using the polar form
/// http://en.wikipedia.org/wiki/Box-Muller_transform


double normrnd(double mu, double sigma){

        static bool deviateAvailable=false;		     //        flag
        static double storedDeviate;                         //        deviate from previous calculation
        double polar, rsquared, var1, var2;

	static int myrnd=0;
	//srand(myrnd);
	//srand((unsigned)time(0));
	//        If no deviate has been stored, the polar Box-Muller transformation is
        //        performed, producing two independent normally-distributed random
        //        deviates.  One is stored for the next round, and one is returned.

        if (!deviateAvailable) {
                //        choose pairs of uniformly distributed deviates, discarding those
                //        that don't fall within the unit circle
                do {
			myrnd = rand();
                        var1=2.0*( double(myrnd)/double(RAND_MAX) ) - 1.0;
			myrnd = rand();
                        var2=2.0*( double(myrnd)/double(RAND_MAX) ) - 1.0;
                        rsquared=var1*var1+var2*var2;
                } while ( rsquared>=1.0 || rsquared == 0.0);

                //        calculate polar tranformation for each deviate
                polar=sqrt(-2.0*log(rsquared)/rsquared);

                //        store first deviate and set flag
                storedDeviate=var1*polar;
                deviateAvailable=true;
             
                //        return second deviate
                return var2*polar*sigma + mu;
        }

        //        If a deviate is available from a previous call to this function, it is
        //        returned, and the flag is set to false.
        else {
                deviateAvailable=false;
                return storedDeviate*sigma + mu;
        }
}


float gauss(int px, int py, float centerx, float centery, float sigma){
	float difx = (px-centerx);
	float dify = (py-centery);
	return (exp(-(difx*difx+dify*dify)/(2.0f*sigma*sigma)));
}

// TODO: test & debug
float gaussLogLikelihood3d(const matrix& mean, const matrix& sigma, const matrix& val){
	matrix inn = val - mean;
	float res = -1.5*log(PIx2) -0.5*log(sigma.det()) -0.5*(inn.transpose()*(sigma.inverse()*inn)).get(0,0);
	return res;
}

// TODO: test & debug
Ematrix gaussianSamplexd(const Ematrix& mean, const Ematrix& sigma){
	// Find any real matrix A such that AA' = Σ. When Σ is positive-definite, the Cholesky decomposition is typically used. 
	// In the more general nonnegative-definite case, one can use the matrix A = UW½ obtained from a spectral decomposition Σ = UWU' of Σ.
	cv::Mat s(sigma.getRows(),sigma.getCols(),CV_32FC1); // sigma
	for (int r = 0; r<sigma.getRows(); r++)				  // copy values to the opencv object
		for (int c = 0; c<sigma.getCols(); c++)
			s.at<float>(r,c) = sigma.get(r,c);

	cv::SVD sigmasvd(s);

	Ematrix U(sigma.getRows(),sigma.getCols());
	for (int r = 0; r<sigma.getRows(); r++)
		for (int c = 0; c<sigma.getCols(); c++)
			U.set(r,c, sigmasvd.u.at<float>(r,c));

	Ematrix L(sigma.getRows(),sigma.getCols());
	for (int r = 0; r<sigma.getRows(); r++)
		L.set(r,r, sqrt(sigmasvd.w.at<float>(r,0)));

	Ematrix A = U*L;

	// Let Z = (z1, …, zN)T be a vector whose components are N independent standard normal variates (which can be generated, for example, by using the Box–Muller transform).
	Ematrix Z(sigma.getRows(),1);
	for (int i = 0; i < sigma.getRows(); i ++)	
		Z.set(i,0,normrnd(0, 1));
	
	// Let X be μ + AZ. This has the desired distribution due to the affine transformation property.
	Ematrix sample = mean + A*Z;

	return sample;
}

bool intersect(line& line1, line& line2, pointf& result){
	//printf("line1 x1: %f, y1: %f, x2: %f, y2: %f\n", line1.x1, line1.y1, line1.x2, line1.y2);
	float A1 = line1.y2 - line1.y1;
	float B1 = line1.x1 - line1.x2;
	float C1 = A1*line1.x1 + B1*line1.y1;

	float A2 = line2.y2 - line2.y1;
	float B2 = line2.x1 - line2.x2;
	float C2 = A2*line2.x1 + B2*line2.y1;

	float det = A1*B2 - A2*B1;
	if (!det) return false;
	else{
		// punto de corte entre las dos rectas
		result.x = (B2*C1 - B1*C2)/det;
	    	result.y = (A1*C2 - A2*C1)/det;    
	}

	const float delta = -0.01f; //Ojo! deberia ser 0.0, por errores de redondeo hay que tomar -0.1 como margen

	// comprobamos que el punto de corte este definido dentro de los 2 segmentos
	return (((line1.x1-result.x)*(result.x-line1.x2)>=delta &&
			 (line1.y1-result.y)*(result.y-line1.y2)>=delta &&
			 (line2.x1-result.x)*(result.x-line2.x2)>=delta &&
			 (line2.y1-result.y)*(result.y-line2.y2)>=delta ) ? true : false ); //no se intersectan
}

matrix::matrix():rows(0),cols(0),val(0){}

matrix::~matrix(){if (val) delete[] val;}

matrix::matrix(int r, int c):	 rows(r),cols(c),val(new float[r*c]){clear();}


matrix::matrix(int r, int c, const float* p):
	rows(r),
	cols(c),
	val(new float[rows*cols])
{
	memcpy(val,p,rows*cols*sizeof(float));
}

matrix::matrix(int r, int c, const double* p):
	rows(r),
	cols(c),
	val(new float[rows*cols])
{
	int total = rows*cols;
	for(int i = 0; i< total; i++) val[i] = p[i];
}


matrix::matrix(const matrix &m):	rows(m.rows),cols(m.cols),val(new float[m.rows*m.cols]){
	memcpy(val, m.val, rows*cols*sizeof(float));
}

matrix::matrix(const pos3d &p):	rows(3), cols(1), val(new float[3]){
	set(0,0,p.getX());
	set(1,0,p.getY());
	set(2,0,p.getZ());
}

matrix::matrix(const pose4 &p):	rows(4), cols(1), val(new float[4]){
	set(0,0,p.x);
	set(1,0,p.y);
	set(2,0,p.z);
	set(2,0,p.th);
}

matrix::matrix(const tf::Pose &p): rows(4), cols(1), val(new float[4]){
	set(0,0,p.getOrigin().getX());
	set(1,0,p.getOrigin().getY());
	set(2,0,p.getOrigin().getZ());
	set(2,0,tf::getYaw(p.getRotation()) );
}

matrix::matrix(const geometry_msgs::Pose &p): rows(4), cols(1), val(new float[4]){
	set(0,0,p.position.x);
	set(1,0,p.position.y);
	set(2,0,p.position.z);
	set(3,0,tf::getYaw(p.orientation) );
}

matrix::matrix(const posCil3d &p):
	rows(3),
	cols(1),
	val(new float[3])
{
	set(0,0,p.getR());
	set(1,0,p.getAlfa());
	set(2,0,p.getZ());
}


void matrix::clear(){
	for (int i = 0; i<rows*cols; i++) val[i] = 0.0f;
}


pos3d matrix::toPos3d() const {
	if(rows != 3 || cols != 1) { printf("ToPos3D: Need to be a 3x1 matrix \n"); }
	assert(rows == 3 && cols == 1);
	pos3d p(get(0,0),get(1,0),get(2,0));
	return p;
}

pose matrix::toPose() const {
	if(rows != 3 || cols != 1) { printf("ToPose: Need to be a 3x1 matrix \n"); }
	assert(rows == 3 && cols == 1);
	return pose(get(0,0),get(1,0),get(2,0));
}
pose4 matrix::toPose4() const {
	if(rows != 4 || cols != 1) { printf("ToPose4: Need to be a 4x1 matrix \n"); }
	assert(rows == 4 && cols == 1);
	return pose4(get(0,0),get(1,0),get(2,0),get(3,0));
}

matrix& matrix::operator=(const matrix& m){
	if(m.rows != rows || m.cols != cols){
		if (val) delete[] val;
		rows = m.rows;
		cols = m.cols;
		val = new float[rows*cols];
	}
	memcpy(val, m.val, rows*cols*sizeof(float));
	return *this;
}

matrix& matrix::operator=(const pos3d& p){
	if(rows != 3 || cols != 1){
		if (val) delete[] val;
		rows = 3;
		cols = 1;
		val = new float[rows*cols];
	}
	set(0,0,p.getX());
	set(1,0,p.getY());
	set(2,0,p.getZ());
	return *this;
}

matrix& matrix::operator=(const posCil3d& p){
	if(rows != 3 || cols != 1){
		if (val) delete[] val;
		rows = 3;
		cols = 1;
		val = new float[rows*cols];
	}
	set(0,0,p.getR());
	set(1,0,p.getAlfa());
	set(2,0,p.getZ());

	return *this;
}

matrix& matrix::operator+= (const matrix& m){
	if(m.rows != rows || m.cols != cols)
		printf("SUM: Matrix dimensions don't agree\n");
	else
		for (int i = 0; i<rows*cols; i++) val[i]+=m.val[i];
	return *this;
}

matrix matrix::operator+(const matrix& m) const{
	matrix mat(*this);
	mat += m;
	return mat;
}

matrix& matrix::operator-= (const matrix& m){
	if(m.rows != rows || m.cols != cols)
		printf("SUM: Matrix dimensions don't agree\n");
	else
		for (int i = 0; i<rows*cols; i++) val[i]-=m.val[i];
	return *this;
}

matrix matrix::operator-(const matrix& m) const {
	matrix mat(*this);
	mat -= m;
	return mat;
}

matrix& matrix::operator*= (const float& f) {
	for (int i = 0; i<rows*cols; i++) val[i]*=f;
	return *this;
}

matrix matrix::operator*(const float& f) const {
	matrix mat(*this);
	mat *= f;
	return mat;
}

matrix matrix::operator*(const matrix& m) const{
	matrix mat(rows, m.cols);
	if (this->cols != m.rows) printf("MUL: Matrix dimensions don't agree\n"); 
	else{
		int id1,id2,id3=0,id4;
		for (int i = 0; i < cols; i++){
			id1 = 0;
			id2 = 0;
			for (int r = 0; r<rows; r++){
				id4 = id2+i;
				if (val[id4]!=0){
					for (int c = 0; c<m.cols; c++){
						mat.val[id1+c] += val[id4] * m.val[id3+c];
					}
				}
				id1 += mat.cols;
				id2 += cols;
			}
			id3 += m.cols;
		}
	}
	return mat;
}

matrix matrix::transpose () const {
	matrix mat(cols,rows);
	for (int r = 0; r<rows; r++)
		for (int c = 0; c<cols; c++)
			mat.set(c,r, val[r*cols+c]);
	return mat;
}

matrix matrix::inverse() const{
	int r;
	if (rows != cols) { printf("INV: Matrix must be square\n");}
	assert(rows == cols);
	matrix mat(rows,cols);
	CvMat* ocvMat = cvCreateMat(rows,cols,CV_32FC1);
	for (r = 0; r<rows; r++)
		for (int c = 0; c<cols; c++)
			cvmSet(ocvMat,r,c,get(r,c));	
	
	CvMat* ocvMatInv = cvCreateMat(rows,cols,CV_32FC1);
	cvInvert(ocvMat, ocvMatInv, CV_LU );
	for (r = 0; r<rows; r++)
		for (int c = 0; c<cols; c++)
			mat.set(r,c, (float)cvmGet(ocvMatInv,r,c));
	cvReleaseMat(&ocvMat);
	cvReleaseMat(&ocvMatInv);
	return mat;
}

float matrix::det() const {
	if(rows != 3 || cols !=3){printf("DET: Only 3x3 determinants");}
	assert(rows == 3 && cols ==3);
	return(get(0,0)*get(1,1)*get(2,2)+get(0,1)*get(1,2)*get(2,0)+get(0,2)*get(1,0)*get(2,1)
		  -get(0,0)*get(1,2)*get(2,1)-get(0,2)*get(1,1)*get(2,0)-get(0,1)*get(1,0)*get(2,2));
}

void matrix::print(const char* str) const {
	printf("%s\t",str);
	for (int r = 0; r<rows; r++){
		for (int c = 0; c<cols; c++)
			printf("%e \t", get(r,c));
			//printf("%10.7f \t", get(r,c));
		printf("\n\t");
	}
	printf("\n");
}

const matrix matrix::identity(int r){
	matrix mat(r,r);
	for (int i=0; i< r; i++) mat.set(i,i,1);
	return mat;
}

void matrix::set(int r, int c, const matrix& mat){
	int x,y;
	x = r;
	for (int i = 0; i < mat.getRows() ; i++){
		y=c;
		for (int j = 0; j < mat.getCols() ; j++){
			set(x,y,mat(i,j));
			y++;
		}
		x++;
	}
}

bool matrix::issymmetric() const{
	for (int i = 0; i < rows ; i++){
		for (int j = 0; j < cols ; j++){
			if(fabs(get(i,j) - get(j,i)) > 0.0000001 ) return false;
		}
	}
	return true;	
}

void matrix::makesymmetric(){
	for (int i = 0; i < rows ; i++){
		for (int j = i; j < cols ; j++){
			set(j,i,get(i,j));
		}
	}
}
////////////////////////////////////////////   Ematrix   ///////////////////////////////////////////////

Ematrix::Ematrix(): reservedRows(0),reservedCols(0),val(0){}

Ematrix::~Ematrix(){if (val) delete[] val;}

Ematrix::Ematrix(int r, int c, int resr, int resc):
	reservedRows(resr),
	reservedCols(resc),
	rows(r),
	cols(c),
	val(new float[reservedRows*reservedCols])
{
	clear();
}

Ematrix::Ematrix(int r, int c):
	reservedRows(r),
	reservedCols(c),
	rows(r),
	cols(c),
	val(new float[reservedRows*reservedCols])
{
	clear();
}

Ematrix::Ematrix(int r, int c, const float* p):
	reservedRows(r),
	reservedCols(c),
	rows(r),
	cols(c),
	val(new float[reservedRows*reservedCols])
{
	memcpy(val,p,rows*cols*sizeof(float));
}

Ematrix::Ematrix(int r, int c, const double* p):
	reservedRows(r),
	reservedCols(c),
	rows(r),
	cols(c),
	val(new float[reservedRows*reservedCols])
{
	int total = rows*cols;
	for(int i = 0; i< total; i++) val[i] = p[i];
}


void Ematrix::initialize(int r, int c, int resr, int resc){
	reservedRows=resr;
	reservedCols=resc;
	rows=r;
	cols=c;
	if (val) delete[] val;
	val = new float[reservedRows*reservedCols];
	clear();
}

Ematrix::Ematrix(const Ematrix &m):	
	reservedRows(m.reservedRows),
	reservedCols(m.reservedCols),
	rows(m.rows),
	cols(m.cols),
	val(new float[reservedRows*reservedCols])
{
	for(int i = 0; i < rows; i++){
		int idx = i*reservedCols;
		for(int j = 0; j < cols; j++){
			int idx2 = idx+j;
			val[idx2] = m.val[idx2];
		}
	}
}

Ematrix::Ematrix(const matrix &m):	
	reservedRows(m.getRows()),
	reservedCols(m.getCols()),
	rows(reservedRows),
	cols(reservedCols),
	val(new float[reservedRows*reservedCols])
{
	for(int i = 0; i < rows; i++){
		int idx = i*reservedCols;
		for(int j = 0; j < cols; j++){
			val[idx+j] = m.get(i,j);
		}
	}
}


Ematrix::Ematrix(const pos3d &p):
	reservedRows(3),
	reservedCols(1),
	rows(3),
	cols(1),
	val(new float[3])
{
	set(0,0,p.getX());
	set(1,0,p.getY());
	set(2,0,p.getZ());
}

Ematrix::Ematrix(const pose4 &p):
	reservedRows(4),
	reservedCols(1),
	rows(4),
	cols(1),
	val(new float[4])
{
	set(0,0,p.x);
	set(1,0,p.y);
	set(2,0,p.z);
	set(3,0,p.th);
}

Ematrix::Ematrix(const geometry_msgs::Pose &p):
	reservedRows(4),
	reservedCols(1),
	rows(4),
	cols(1),
	val(new float[4])
{
	set(0,0,p.position.x);
	set(1,0,p.position.y);
	set(2,0,p.position.z);
	set(3,0,tf::getYaw(p.orientation) );
}

Ematrix::Ematrix(const posCil3d &p):
	reservedRows(3),
	reservedCols(1),
	rows(3),
	cols(1),
	val(new float[3])
{
	set(0,0,p.getR());
	set(1,0,p.getAlfa());
	set(2,0,p.getZ());
}

void Ematrix::clear(){
	int id;
	for(int i = 0; i < rows; i++){
		id = i*reservedCols;
		for(int j = 0; j < cols; j++)
			val[id+j]= 0.0f;
	}
}

Ematrix& Ematrix::operator=(const Ematrix& m){
	if(m.rows > reservedRows || m.cols > reservedCols){
		reservedRows = m.rows;
		reservedCols = m.cols;
		if (val) delete [] val;
		val = new float[reservedRows*reservedCols];
	}
	rows = m.rows;
	cols = m.cols;
	for(int i = 0; i < rows; i++)
		for(int j = 0; j < cols; j++)
			set(i,j, m.get(i,j));
	return *this;
}

Ematrix& Ematrix::operator=(const pos3d& p){
	if(reservedRows < 3 || reservedCols < 1){
		if (val) delete[] val;
		rows = 3;
		cols = 1;
		reservedRows = 3;
		reservedCols = 1;
		val = new float[reservedRows*reservedCols];
	}
	set(0,0,p.getX());
	set(1,0,p.getY());
	set(2,0,p.getZ());

	return *this;
}

Ematrix& Ematrix::operator=(const pose4& p){
	if(reservedRows < 4 || reservedCols < 1){
		if (val) delete[] val;
		rows = 4;
		cols = 1;
		reservedRows = 4;
		reservedCols = 1;
		val = new float[reservedRows*reservedCols];
	}
	set(0,0,p.x);
	set(1,0,p.y);
	set(2,0,p.z);
	set(3,0,p.th);

	return *this;
}

Ematrix& Ematrix::operator=(const tf::Pose& p){
	if(reservedRows < 4 || reservedCols < 1){
		if (val) delete[] val;
		rows = 4;
		cols = 1;
		reservedRows = 4;
		reservedCols = 1;
		val = new float[reservedRows*reservedCols];
	}
	set(0,0,p.getOrigin().getX());
	set(1,0,p.getOrigin().getY());
	set(2,0,p.getOrigin().getZ());
	set(3,0,tf::getYaw(p.getRotation()) );

	return *this;
}


Ematrix& Ematrix::operator=(const geometry_msgs::Pose& p){
	if(reservedRows < 4 || reservedCols < 1){
		if (val) delete[] val;
		rows = 4;
		cols = 1;
		reservedRows = 4;
		reservedCols = 1;
		val = new float[reservedRows*reservedCols];
	}
	set(0,0,p.position.x);
	set(1,0,p.position.y);
	set(2,0,p.position.z);
	set(3,0,tf::getYaw(p.orientation) );

	return *this;
}

Ematrix& Ematrix::operator=(const posCil3d& p){
	if(reservedRows < 3 || reservedCols < 1){
		if (val) delete[] val;
		rows = 3;
		cols = 1;
		reservedRows = 3;
		reservedCols = 1;
		val = new float[reservedRows*reservedCols];
	}
	set(0,0,p.getR());
	set(1,0,p.getAlfa());
	set(2,0,p.getZ());
	return *this;
}

Ematrix Ematrix::operator+(const Ematrix& m) const{
	Ematrix mat(*this);
	mat += m;
	return mat;
}

Ematrix Ematrix::operator-(const Ematrix& m) const{
	Ematrix mat(*this);
	mat -= m;
	return mat;
}

Ematrix& Ematrix::operator+= (const Ematrix& m){
	if(m.rows != rows || m.cols != cols)
		printf("SUM: Matrix dimensions don't agree\n");
	else{
		int id=0,id2=0;
		for(int i = 0; i < rows; i++){
			for(int j = 0; j < cols; j++)
				val[id+j] += m.val[id2+j];
			id += reservedCols;
			id2 += m.reservedCols;
		}
	}
	return *this;
}

Ematrix& Ematrix::operator-= (const Ematrix& m){
	if(m.rows != rows || m.cols != cols)
		printf("SUM: Matrix dimensions don't agree\n");
	else{
		int id=0,id2=0;
		for(int i = 0; i < rows; i++){
			for(int j = 0; j < cols; j++)
				val[id+j] -= m.val[id2+j];
			id += reservedCols;
			id2 += m.reservedCols;
		}
	}
	return *this;
}

Ematrix& Ematrix::operator*= (const float& f) {
	int id=0;
	for(int i = 0; i < rows; i++){
		for(int j = 0; j < cols; j++) 
			val[id+j] *= f;
		id += reservedCols;
	}
	return *this;
}

Ematrix Ematrix::operator*(const float& f) const {
	Ematrix mat(*this);
	mat *= f;
	return mat;
}

Ematrix Ematrix::operator*(const Ematrix& m) const{
	Ematrix mat(rows, m.cols, reservedRows, m.reservedCols);
	if (this->cols != m.rows) printf("MUL: Matrix dimensions don't agree\n"); 
	else{
		int id, id1, id2=0, id3;
		for (int i = 0; i < cols; i++){
			id = 0;
			id3 = 0;
			for (int r = 0; r<rows; r++){
				id1 = id3+i;
				if (val[id1]!=0){
					for (int c = 0; c<m.cols; c++){
						mat.val[id+c] += val[id1] * m.val[id2+c];
					}
				}
				id3 +=reservedCols;
				id += mat.reservedCols;
			}
			id2 += m.reservedCols;
		}
	}
	return mat;
}

Ematrix Ematrix::mul2(const Ematrix& m) const{
	Ematrix mat(rows, m.cols, reservedRows, m.reservedCols);
	if (this->cols != m.rows) printf("MUL: Matrix dimensions don't agree\n"); 
	else{
		int id=0, id1, id2, id3;
		for (int i = 0; i < cols; i++){
			for (int c = 0; c<m.cols; c++){
				id1 = id+c;
				if (m.val[id1]!=0){
					id2 = 0; id3 = 0;
					for (int r = 0; r<rows; r++){
						mat.val[id3+c] += val[id2+i] * m.val[id1];
						id2+=reservedCols;
						id3+=mat.reservedCols;
					}
				}
			}
			id +=m.reservedCols;				
		}
	}
	return mat;
}

Ematrix Ematrix::transpose () const {
	Ematrix mat(cols,rows,reservedCols,reservedRows);
	int id=0;
	for (int r = 0; r<rows; r++){
		for (int c = 0; c<cols; c++)
			mat.set(c,r, val[id+c]);
		id += reservedCols;		
	}
	return mat;
}

Ematrix Ematrix::inverse() const{
	int r,c;
	if (rows != cols) { printf("INV: Matrix must be square\n");}
	assert(rows == cols);
	Ematrix mat(rows,cols,reservedRows,reservedCols);
	CvMat* ocvMat = cvCreateMat(rows,cols,CV_32FC1);
	int id=0;
	for (r = 0; r<rows; r++){
		for ( c = 0; c<cols; c++)
			cvmSet(ocvMat,r,c, val[id+c]);	
		id += reservedCols;
	}

	CvMat* ocvMatInv = cvCreateMat(rows,cols,CV_32FC1);
	cvInvert(ocvMat, ocvMatInv, CV_LU );
	id = 0;
	for (r = 0; r<rows; r++){
		for (c = 0; c<cols; c++)
			mat.val[id+c] = (float)cvmGet(ocvMatInv,r,c);
		id += reservedCols;
	}
	cvReleaseMat(&ocvMat);
	cvReleaseMat(&ocvMatInv);
	return mat;
}

float Ematrix::det() const {
//	if(rows != 3 || cols !=3){printf("DET: Only 3x3 determinants");}
//	assert(rows == 3 && cols ==3);
//	return(get(0,0)*get(1,1)*get(2,2)+get(0,1)*get(1,2)*get(2,0)+get(0,2)*get(1,0)*get(2,1)
//		  -get(0,0)*get(1,2)*get(2,1)-get(0,2)*get(1,1)*get(2,0)-get(0,1)*get(1,0)*get(2,2));
	cv::Mat m = cv::Mat::zeros(rows, cols, CV_32FC1) ;
	for (int r = 0; r<rows; r++)
		for (int c = 0; c<cols; c++)
			m.at<float>(r,c) = get(r,c);
	return (cv::determinant(m));	
}

void Ematrix::print(const char* str) const {
	printf("%s\t",str);
	for (int r = 0; r<rows; r++){
		for (int c = 0; c<cols; c++)
			printf("%10.7f \t", get(r,c));
			//printf("%e \t", get(r,c));
		printf("\n\t");
	}
	printf("\n");
}

const Ematrix Ematrix::identity(int r){
	Ematrix mat(r,r,r,r);
	for (int i=0; i< r; i++) mat.set(i,i,1);
	return mat;
}

pos3d Ematrix::toPos3d() const {
	if(rows != 3 || cols != 1) { printf("ToPos3D: Need to be a 3x1 matrix \n"); }
	assert(rows == 3 && cols == 1);
	return pos3d(get(0,0),get(1,0),get(2,0));
}

pose Ematrix::toPose() const {
	if(rows != 3 || cols != 1) { printf("ToPose: Need to be a 3x1 matrix \n"); }
	assert(rows == 3 && cols == 1);
	return pose(get(0,0),get(1,0),get(2,0));
}
pose4 Ematrix::toPose4() const {
	if(rows != 4 || cols != 1) { printf("ToPose4: Need to be a 4x1 matrix \n"); }
	assert(rows == 4 && cols == 1);
	return pose4(get(0,0),get(1,0),get(2,0),get(3,0));
}

Ematrix Ematrix::subMat(int rini, int rend, int cini, int cend) const{
	int sizex = rend-rini+1;
	int sizey = cend-cini+1;
	Ematrix mat (sizex, sizey);
	int i,j;
	i=0;
//	printf("Submat [%d:%d, %d:%d]\n",rini,rend,cini,cend);
//	printf("(%d,%d) res (%d,%d)",rows,cols,reservedRows,reservedCols);
	for (int r = rini; r<= rend; r++){
		j=0;
		for (int c = cini; c<=cend; c++){
			mat.set(i,j,get(r,c));
//			printf("[%d,%d] %f ",r,c,get(r,c));
			j++;
		}
		i++;
//		printf("\n");
	}
//	printf("\n");
	return mat;
}

void Ematrix::extend(int rinc, int cinc){
	//printf("reserved: %d x %d\n",reservedRows,reservedCols);
	//printf("size: %d x %d\n",rows, cols);
	if(reservedRows < rows+rinc || reservedCols < cols+cinc){
		int newreservedRows = rows+rinc;
		int newreservedCols = cols+cinc;
		float* newval = new float[newreservedRows*newreservedCols];

		for(int i = 0; i < rows; i++)
			for(int j = 0; j < cols; j++)
				newval[i*newreservedCols+j] = val[i*reservedCols+j];

		delete [] val;
		val = newval;
		reservedRows = newreservedRows;
		reservedCols = newreservedCols;
	}
	int oldcols = cols;
	int oldrows = rows;
	cols+=cinc;
	rows+=rinc;
	for (int r = oldrows; r< rows; r++)
		for (int c = 0; c< oldcols; c++)	
			set(r,c,0.0f);

	for (int r = 0; r< rows; r++)
		for (int c = oldcols; c<cols; c++)
			set(r,c,0.0f);
}

void Ematrix::set(int r, int c, const matrix& mat){
	int x,y;
	x=r;
	for (int i = 0; i < mat.getRows() ; i++){
		y=c;
		for (int j = 0; j < mat.getCols() ; j++){
			set(x,y,mat(i,j));
			y++;
		}
		x++;
	}
}

void Ematrix::set(int r, int c, const Ematrix& mat){
	int x,y;
	x=r;
	for     (int i = 0; i < mat.rows ; i++){
		y=c;
		for (int j = 0; j < mat.cols ; j++){
			set(x,y,mat(i,j));
			y++;
		}
		x++;
	}
}

void Ematrix::addSubMat(int r, int c, const matrix& mat){
	int x,y;
	x=r;
	for (int i = 0; i < mat.getRows() ; i++){
		y=c;
		for (int j = 0; j < mat.getCols() ; j++){
			set(x,y,get(x,y)+mat(i,j));
			y++;
		}
		x++;
	}
}


float Ematrix::totalsum() const{
	float res = 0.0f;
	for (int i = 0; i < rows ; i++){
		for (int j = 0; j < cols ; j++){
			res += fabs(get(i,j));
		}
	}
	return res;
}

void Ematrix::makesymmetric(){
	for (int i = 0; i < rows ; i++){
		for (int j = i; j < cols ; j++){
			set(j,i,get(i,j));
		}
	}
}


int sign(int x){ return x > 0 ? 1.0 : -1.0; }

void bresenham2D(unsigned int abs_da, unsigned int abs_db, int dir, int error_b, int p_x, int p_y, int offset_a, int offset_b, unsigned int max_length, std::vector<geometry_msgs::Point>& cells){
	int end = std::min(max_length, abs_da);
	for(int i = 0; i < end; ++i){
		geometry_msgs::Point p;
		if (dir){	p.x=p_x; p.y=p_y;}
		else	{	p.x=p_y; p.y=p_x;}
		cells.push_back(p);
		p_x += offset_a;
		error_b += abs_db;
		if(error_b >= abs_da){
			p_y += offset_b;
			error_b -= abs_da;
		}
	}
	geometry_msgs::Point p;
	if (dir){	p.x=p_x; p.y=p_y;}
	else	{	p.x=p_y; p.y=p_x;}
	cells.push_back(p);
	//ROS_INFO("bresenham: cells size %d", cells.size());
}

void raytraceLine(int x0, int y0, int x1, int y1, std::vector<geometry_msgs::Point>& cells, unsigned int max_length){
  
	int dx = x1 - x0;			// x distance
	int dy = y1 - y0;			// y distance

	unsigned int abs_dx = abs(dx);		// abs x distance
	unsigned int abs_dy = abs(dy);		// abs y distance

	int offset_dx = sign(dx);		// direction in x ax			
	int offset_dy = sign(dy);		// direction in y ax

	double dist = sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1)); // euclidean dist p0 to p1

	//we need to chose how much to scale our dominant dimension, based on the maximum length of the line
	double scale = std::min(1.0,  max_length / dist); 

	//if x is dominant
	if(abs_dx >= abs_dy){
		int error_y = abs_dx / 2; //???
		bresenham2D(abs_dx, abs_dy, 1, error_y, x0, y0, offset_dx, offset_dy, (unsigned int)(scale * abs_dx), cells);
  	}
	else{ //otherwise y is dominant
  		int error_x = abs_dy / 2;
  		bresenham2D(abs_dy, abs_dx, 0, error_x, y0, x0, offset_dy, offset_dx, (unsigned int)(scale * abs_dy), cells);
	}
	//ROS_INFO("raytrace: cells size %d", cells.size());

}

point* getLine(int x1, int y1, int x2, int y2, uint& points){
	int i,j;
	uint k;

	float incx, incy, auxx, auxy;
	auxx = fabs((float)(x1-x2))+0.0001;
	auxy = fabs((float)(y1-y2))+0.0001;
	incx = auxx/auxy;
	incy = auxy/auxx;

	int count = 0;
	point* pointbuf;

	if (incx > incy){
		j = y1;

		points = (int)(auxx+1);
		pointbuf = new point[points]();
		count=1;
		if (x1 < x2){
			for(i = x1,k=0; i <= x2; i++,k++){
				if (fabs((float)(i-x1))>=count*incx) {
					count++;
					(y1<y2)? j++ : j--;
				}
				pointbuf[k].x = i;
				pointbuf[k].y = j;
			}
		}
		else{
			for(i = x1,k=0; i >= x2; i--,k++){
				if (fabs((float)(x1-i))>=count*incx) {
					count++;
					(y1<y2)? j++ : j--;
				}
				pointbuf[k].x = i;
				pointbuf[k].y = j;
			}
		}
	}
	
	else{
		j = x1;
		points = (int)(auxy+1);
		pointbuf = new point[points]();

		count=1;
		if (y1 < y2){
			for(i = y1,k=0; i <= y2; i++,k++){
				if (fabs((float)(i-y1))>=count*incy) {
					count++;
					(x1<x2)? j++ : j--;
				}
				pointbuf[k].x = j;
				pointbuf[k].y = i;
			}
		}
		else{
			for(i = y1,k=0; i >= y2; i--,k++){
				if (fabs((float)(y1-i))>=count*incy) {
					count++;
					(x1<x2)? j++ : j--;
				}
				pointbuf[k].x = j;
				pointbuf[k].y = i;
			}
		}
	}
	//if (points != k) printf("error!!!\n");
	return pointbuf;
}


