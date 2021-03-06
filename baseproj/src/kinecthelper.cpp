#include "stdafx.h"
#include "DXApp.h"

WAITABLE_HANDLE hColorWaitable;

bool KinectHelper::initKinect()
{

	HRESULT hr;
	hColorWaitable = 0;
	hr = GetDefaultKinectSensor(&m_pKinectSensor);

	//KinectSensor::GetDefault();
	m_orientFilter.reset();

	
	m_orientFilter.m_smoothParameters.Smoothing = 0.5f;
	m_orientFilter.m_smoothParameters.Correction = 0.15f;
	m_orientFilter.m_smoothParameters.Prediction = 0.25f;
	m_orientFilter.m_smoothParameters.JitterRadius = 0.1f;
	m_orientFilter.m_smoothParameters.MaxDeviationRadius = 0.1f;


	for (int i = 0; i < JointType_Count; i++)
	{
		m_boneMapping[i].boneIndex = -1;
	}

	if (FAILED(hr))
	{
		return false;
	}

	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get coordinate mapper and the body reader
		IBodyFrameSource* pBodyFrameSource = NULL;

		hr = m_pKinectSensor->Open();

	
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);

			if (SUCCEEDED(hr))
			{
				hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
				
				if (SUCCEEDED(hr))
				{
					hr = pBodyFrameSource->SubscribeFrameCaptured(&hColorWaitable);
					if (FAILED(hr)){

						TRACE("Failed to initialize DirectX\n");
					}

				}


			}

		}

		SafeRelease(pBodyFrameSource);
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		return false;
	}

	m_initialized = true;

	return true;
}


bool KinectHelper::destroyKinect()
{

	// done with body frame reader
	SafeRelease(m_pBodyFrameReader);

	// close the Kinect Sensor
	if (m_pKinectSensor)
	{
		m_pKinectSensor->Close();
	}

	SafeRelease(m_pKinectSensor);
	m_initialized = false;

	return true;

}



void KinectHelper::setBoneMapping(JointType jointType, int boneIndex)
{

	m_boneMapping[jointType].boneIndex = boneIndex;
	
}


void KinectHelper::mapBones(std::vector<Ogre::Euler>& eulers)
{

	for (int i = 0; i < m_boneMapping.size();i++)
	{

		int index = m_boneMapping[i].boneIndex;

		if (index >= 0)
		{
			Ogre::Euler euler(m_boneMapping[i].ori);

			eulers[index] = euler;

		}
	
	}

}


void KinectHelper::update()
{

	if (!m_pBodyFrameReader)
	{
		return;
	}


	//wait for frame 2sec time out
	HANDLE hEvents[] = { reinterpret_cast<HANDLE>(hColorWaitable) };
	DWORD  res = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, true, 2000);
	
	if (res == WAIT_TIMEOUT)
	{
	
		return;  
	}


	IBodyFrame* pBodyFrame = NULL;

	HRESULT hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);

	if (SUCCEEDED(hr))
	{
		INT64 nTime = 0;

		hr = pBodyFrame->get_RelativeTime(&nTime);

		IBody* ppBodies[BODY_COUNT] = { 0 };

		if (SUCCEEDED(hr))
		{
			hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
		}

		if (SUCCEEDED(hr))
		{
			processBody(BODY_COUNT, ppBodies);
		}

		for (int i = 0; i < _countof(ppBodies); ++i)
		{
			SafeRelease(ppBodies[i]);
		}
	}

	SafeRelease(pBodyFrame);
}

void KinectHelper::processBody(int nBodyCount, IBody** ppBodies)
{

	HRESULT hr;
	m_lastFrameOk = false;

	
	for (int i = 0; i < nBodyCount; ++i)
	{
		IBody* pBody = ppBodies[i];
		if (pBody)
		{
			BOOLEAN bTracked = false;
			hr = pBody->get_IsTracked(&bTracked);

			if (SUCCEEDED(hr) && bTracked)
			{
				Joint joints[JointType_Count];
				JointOrientation oriens[JointType_Count];
				
				//HandState leftHandState = HandState_Unknown;
				//HandState rightHandState = HandState_Unknown;

				// pBody->get_HandLeftState(&leftHandState);
				// pBody->get_HandRightState(&rightHandState);

				hr = pBody->GetJoints(_countof(joints), joints);
		
				//if (SUCCEEDED(hr))
				//{
				hr = pBody->GetJointOrientations(_countof(oriens), oriens);
				
				if (SUCCEEDED(hr))
				{

					for (int j = 0; j < _countof(oriens); ++j)
					{ 

						Ogre::Quaternion ori = Ogre::Quaternion(
							oriens[j].Orientation.w,
							oriens[j].Orientation.x,
							oriens[j].Orientation.y,
							oriens[j].Orientation.z);

						Ogre::Vector3 vec = Ogre::Vector3(joints[j].Position.X, joints[j].Position.Y, joints[j].Position.Z);


//							m_boneMapping[j].pos = vec;
						m_boneMapping[j].ori = ori;						
						m_boneMapping[j].pos = vec;
						m_boneMapping[j].state = joints[j].TrackingState;

						m_lastFrameOk = true;
					
						//filter the orientations
										
					
					}
					
					if (m_lastFrameOk)
					{
						m_orientFilter.filter(m_boneMapping, m_boneMappingFiltered);
					};
				}
				//}
			}
		}

		if (m_lastFrameOk)
			break;
	}

			
}





