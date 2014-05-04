#include <math.h>
#include <stdint.h>

#pragma once

#define GRAVITY_MSS 9.80665f
#define deg2rad 0.017453292f
#define rad2deg 57.295780f
#define pi 3.141592657f
#define earthRate 0.000072921f
#define earthRadius 6378145.0f
#define earthRadiusInv  1.5678540e-7f

class Vector3f
{
private:
public:
    float x;
    float y;
    float z;

    float length(void) const;
    void zero(void);
};

class Mat3f
{
private:
public:
    Vector3f x;
    Vector3f y;
    Vector3f z;

    Mat3f();

    Mat3f transpose(void) const;
};

Vector3f operator*(float sclIn1, Vector3f vecIn1);
Vector3f operator+( Vector3f vecIn1, Vector3f vecIn2);
Vector3f operator-( Vector3f vecIn1, Vector3f vecIn2);
Vector3f operator*( Mat3f matIn, Vector3f vecIn);
Vector3f operator%( Vector3f vecIn1, Vector3f vecIn2);
Vector3f operator*(Vector3f vecIn1, float sclIn1);

void swap_var(float &d1, float &d2);

const unsigned int n_states = 24;
const unsigned int n_storedStates = 27;
const unsigned int data_buffer_size = 50;

enum GPS_FIX {
    GPS_FIX_NOFIX = 0,
    GPS_FIX_2D = 2,
    GPS_FIX_3D = 3
};

struct ekf_status_report {
    bool velHealth;
    bool posHealth;
    bool hgtHealth;
    bool velTimeout;
    bool posTimeout;
    bool hgtTimeout;
    uint32_t velFailTime;
    uint32_t posFailTime;
    uint32_t hgtFailTime;
    float states[n_states];
    bool statesNaN;
    bool covarianceNaN;
    bool kalmanGainsNaN;
};

class AttPosEKF {

public:

    AttPosEKF();
    ~AttPosEKF();



    /* ##############################################
     *
     *   M A I N    F I L T E R    P A R A M E T E R S
     *
     * ########################################### */

    /*
     * parameters are defined here and initialised in
     * the InitialiseParameters() (which is just 20 lines down)
     */

    float covTimeStepMax; // maximum time allowed between covariance predictions
    float covDelAngMax; // maximum delta angle between covariance predictions
    float rngFinderPitch; // pitch angle of laser range finder in radians. Zero is aligned with the Z body axis. Positive is RH rotation about Y body axis.
    float a1; // optical flow sensor misalgnment angle about X axis (rad)
    float a2; // optical flow sensor misalgnment angle about Y axis (rad)
    float a3; // optical flow sensor misalgnment angle about Z axis (rad)

    float yawVarScale;
    float windVelSigma;
    float dAngBiasSigma;
    float dVelBiasSigma;
    float magEarthSigma;
    float magBodySigma;
    float gndHgtSigma;
    float optScaleSigma;

    float vneSigma;
    float vdSigma;
    float posNeSigma;
    float posDSigma;
    float magMeasurementSigma;
    float airspeedMeasurementSigma;

    float gyroProcessNoise;
    float accelProcessNoise;

    float EAS2TAS; // ratio f true to equivalent airspeed

    void InitialiseParameters()
    {
        covTimeStepMax = 0.07f; // maximum time allowed between covariance predictions
        covDelAngMax = 0.02f; // maximum delta angle between covariance predictions
        rngFinderPitch = 0.0f; // pitch angle of laser range finder in radians. Zero is aligned with the Z body axis. Positive is RH rotation about Y body axis.
        a1 = 0.0f; // optical flow sensor misalgnment angle about X axis (rad)
        a2 = 0.0f; // optical flow sensor misalgnment angle about Y axis (rad)
        a3 = 0.0f; // optical flow sensor misalgnment angle about Z axis (rad)

        EAS2TAS = 1.0f;

        yawVarScale = 1.0f;
        windVelSigma = 0.1f;
        dAngBiasSigma = 5.0e-7f;
        dVelBiasSigma = 1e-4f;
        magEarthSigma = 3.0e-4f;
        magBodySigma  = 3.0e-4f;
        gndHgtSigma  = 0.02f; // assume 2% terrain gradient 1-sigma
        optScaleSigma = 1e-4;

        vneSigma = 0.2f;
        vdSigma = 0.3f;
        posNeSigma = 2.0f;
        posDSigma = 2.0f;

        magMeasurementSigma = 0.05;
        airspeedMeasurementSigma = 1.4f;
        gyroProcessNoise = 1.4544411e-2f;
        accelProcessNoise = 0.5f;
    }




    // Global variables
    float KH[n_states][n_states]; //  intermediate result used for covariance updates
    float KHP[n_states][n_states]; // intermediate result used for covariance updates
    float P[n_states][n_states]; // covariance matrix
    float Kfusion[n_states]; // Kalman gains
    float states[n_states]; // state matrix
    float storedStates[n_storedStates][data_buffer_size]; // state vectors stored for the last 50 time steps
    uint32_t statetimeStamp[data_buffer_size]; // time stamp for each state vector stored

    float statesAtVelTime[n_storedStates]; // States and delta angles at the effective measurement time for posNE and velNED measurements
    float statesAtPosTime[n_storedStates]; // States and delta angles at the effective measurement time for posNE and velNED measurements
    float statesAtHgtTime[n_storedStates]; // States and delta angles at the effective measurement time for the hgtMea measurement
    float statesAtMagMeasTime[n_storedStates]; // States and delta angles at the effective measurement time
    float statesAtVtasMeasTime[n_storedStates]; // States and delta angles at the effective Vtas measurement time
    float statesAtRngTime[n_storedStates]; // States and delta angles at the effective range finder measurement time
    float statesAtLosMeasTime[n_storedStates]; // States and delta angles at the effective optical flow measurement time

    Vector3f correctedDelAng; // delta angles about the xyz body axes corrected for errors (rad)
    Vector3f correctedDelVel; // delta velocities along the XYZ body axes corrected for errors (m/s)
    Vector3f summedDelAng; // summed delta angles about the xyz body axes corrected for errors (rad)
    Vector3f summedDelVel; // summed delta velocities along the XYZ body axes corrected for errors (m/s)
    float accNavMag; // magnitude of navigation accel (- used to adjust GPS obs variance (m/s^2)
    Vector3f earthRateNED; // earths angular rate vector in NED (rad/s)
    Vector3f angRate; // angular rate vector in XYZ body axes measured by the IMU (rad/s)
    float delAngForFusion[3]; // corrected and filtered delta angle vector used for fusion purposes

    Mat3f Tbn; // transformation matrix from body to NED coordinates
    Mat3f Tnb; // transformation amtrix from NED to body coordinates

    Vector3f accel; // acceleration vector in XYZ body axes measured by the IMU (m/s^2)
    Vector3f dVelIMU;
    Vector3f dAngIMU;
    float dtIMU; // time lapsed since the last IMU measurement or covariance update (sec)
    uint8_t fusionModeGPS; // 0 = GPS outputs 3D velocity, 1 = GPS outputs 2D velocity, 2 = GPS outputs no velocity
    float innovVelPos[6]; // innovation output
    float varInnovVelPos[6]; // innovation variance output

    float velNED[3]; // North, East, Down velocity obs (m/s)
    float posNE[2]; // North, East position obs (m)
    float hgtMea; //  measured height (m)
    float rngMea; // Ground distance
    float posNED[3]; // North, East Down position (m)

    float innovMag[3]; // innovation output for magnetometer measurements
    float varInnovMag[3]; // innovation variance output for magnetometer measurements
    float varInnovLOS[2]; // innovation variance output for optical flow measurements
    Vector3f magData; // magnetometer flux radings in X,Y,Z body axes
    float innovVtas; // innovation output for true airspeed measurements
    float innovRng; ///< Range finder innovation for rnge finder measurements
    float innovLOS[2]; // Innovations for optical flow LOS rate measurements
    float losData[2]; // Optical flow LOS rate measurements
    float losPred[2]; // Predicted optical flow measurements
    float varInnovVtas; // innovation variance output
    float VtasMeas; // true airspeed measurement (m/s)
    double latRef; // WGS-84 latitude of reference point (rad)
    double lonRef; // WGS-84 longitude of reference point (rad)
    float hgtRef; // WGS-84 height of reference point (m)
    Vector3f magBias; // states representing magnetometer bias vector in XYZ body axes
    unsigned covSkipCount; // Number of state prediction frames (IMU daya updates to skip before doing the covariance prediction

    // GPS input data variables
    float gpsCourse;
    float gpsVelD;
    double gpsLat;
    double gpsLon;
    float gpsHgt;
    uint8_t GPSstatus;

    // Baro input
    float baroHgt;

    bool statesInitialised;
    bool terrainInitialised;

    bool fuseVelData; // this boolean causes the posNE and velNED obs to be fused
    bool fusePosData; // this boolean causes the posNE and velNED obs to be fused
    bool fuseHgtData; // this boolean causes the hgtMea obs to be fused
    bool fuseMagData; // boolean true when magnetometer data is to be fused
    bool fuseVtasData; // boolean true when airspeed data is to be fused
    bool fuseRngData;   ///< true when range data is fused
    bool fuseOptData;   // true when optical flow data is fused

    bool onGround;    ///< boolean true when the flight vehicle is on the ground (not flying)
    bool staticMode;    ///< boolean true if no position feedback is fused
    bool useAirspeed;    ///< boolean true if airspeed data is being used
    bool useCompass;    ///< boolean true if magnetometer data is being used
    bool useRangeFinder;     ///< true when rangefinder is being used
    bool useOpticalFlow; // true when optical flow data is being used

    struct ekf_status_report current_ekf_state;
    struct ekf_status_report last_ekf_error;

    bool numericalProtection;

    unsigned storeIndex;


void  UpdateStrapdownEquationsNED();

void CovariancePrediction(float dt);

void FuseVelposNED();

void FuseMagnetometer();

void FuseAirspeed();

void ResetTerrain();

void FuseRangeFinder();

void FuseOpticalFlow(float dt);

void zeroRows(float (&covMat)[n_states][n_states], uint8_t first, uint8_t last);

void zeroCols(float (&covMat)[n_states][n_states], uint8_t first, uint8_t last);

void quatNorm(float (&quatOut)[4], const float quatIn[4]);

// store states along with system time stamp in msces
void StoreStates(uint64_t timestamp_ms);

/**
 * Recall the state vector.
 *
 * Recalls the vector stored at closest time to the one specified by msec
 *
 * @return zero on success, integer indicating the number of invalid states on failure.
 *         Does only copy valid states, if the statesForFusion vector was initialized
 *         correctly by the caller, the result can be safely used, but is a mixture
 *         time-wise where valid states were updated and invalid remained at the old
 *         value.
 */
int RecallStates(float *statesForFusion, uint64_t msec);

int RecallDelAng(float *delAngForFusion, uint64_t msec);

void ResetStoredStates();

void quat2Tbn(Mat3f &Tbn, const float (&quat)[4]);

void calcEarthRateNED(Vector3f &omega, float latitude);

static void eul2quat(float (&quat)[4], const float (&eul)[3]);

static void quat2eul(float (&eul)[3], const float (&quat)[4]);

static void calcvelNED(float (&velNED)[3], float gpsCourse, float gpsGndSpd, float gpsVelD);

static void calcposNED(float (&posNED)[3], double lat, double lon, float hgt, double latRef, double lonRef, float hgtRef);

static void calcLLH(float (&posNED)[3], float lat, float lon, float hgt, float latRef, float lonRef, float hgtRef);

static void quat2Tnb(Mat3f &Tnb, const float (&quat)[4]);

static float sq(float valIn);

void OnGroundCheck();

void CovarianceInit();

void InitialiseFilter(float (&initvelNED)[3]);

float ConstrainFloat(float val, float min, float max);

void ConstrainVariances();

void ConstrainStates();

void ForceSymmetry();

int CheckAndBound();

void ResetPosition();

void ResetVelocity();

void ZeroVariables();

void GetFilterState(struct ekf_status_report *state);

void GetLastErrorState(struct ekf_status_report *last_error);

bool StatesNaN(struct ekf_status_report *err_report);
void FillErrorReport(struct ekf_status_report *err);

void InitializeDynamic(float (&initvelNED)[3]);

protected:

bool FilterHealthy();

void ResetHeight(void);

void AttitudeInit(float ax, float ay, float az, float mx, float my, float mz, float *initQuat);

};

uint32_t millis();

