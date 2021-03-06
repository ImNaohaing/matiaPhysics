#include "PhysicsClient.h"
#include "PosixSharedMemory.h"
#include "Win32SharedMemory.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "Bullet3Common/b3Logging.h"
#include "../Utils/b3ResourcePath.h"
#include "../../Extras/Serialize/BulletFileLoader/btBulletFile.h"
#include "../../Extras/Serialize/BulletFileLoader/autogenerated/bullet.h"


struct PhysicsClientSharedMemoryInternalData
{
	SharedMemoryInterface* m_sharedMemory;
	SharedMemoryExampleData*   m_testBlock1;

	int m_counter;
	bool m_serverLoadUrdfOK;
	bool m_isConnected;
	bool m_waitingForServer;
	
	PhysicsClientSharedMemoryInternalData()
		:m_sharedMemory(0),
		m_testBlock1(0),
		m_counter(0),
		m_serverLoadUrdfOK(false),
		m_isConnected(false),
		m_waitingForServer(false)
	{
	}

	

	void	processServerStatus();
	
	bool	canSubmitCommand() const;
	

};

PhysicsClientSharedMemory::PhysicsClientSharedMemory()

{
	m_data = new PhysicsClientSharedMemoryInternalData;	

#ifdef _WIN32
	m_data->m_sharedMemory = new Win32SharedMemoryClient();
#else
	m_data->m_sharedMemory = new PosixSharedMemory();
#endif

}

PhysicsClientSharedMemory::~PhysicsClientSharedMemory()
{
	m_data->m_sharedMemory->releaseSharedMemory(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE);
	delete m_data->m_sharedMemory;
	delete m_data;
}

bool	PhysicsClientSharedMemory::isConnected() const
{
	return m_data->m_isConnected ;
}

bool PhysicsClientSharedMemory::connect(bool allowSharedMemoryInitialization)
{
	bool allowCreation = true;
	m_data->m_testBlock1 = (SharedMemoryExampleData*)m_data->m_sharedMemory->allocateSharedMemory(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, allowCreation);
	
    if (m_data->m_testBlock1)
    {
        if (m_data->m_testBlock1->m_magicId !=SHARED_MEMORY_MAGIC_NUMBER)
        {
			if (allowSharedMemoryInitialization)
			{
				InitSharedMemoryExampleData(m_data->m_testBlock1);
				b3Printf("Created and initialized shared memory block");
				m_data->m_isConnected = true;
			} else
			{
				b3Error("Error: please start server before client\n");
				m_data->m_sharedMemory->releaseSharedMemory(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE);
				m_data->m_testBlock1 = 0;
				return false;
			}
        } else
		{
			b3Printf("Connected to existing shared memory, status OK.\n");
			m_data->m_isConnected = true;
		}
    } else
	{
		b3Error("Cannot connect to shared memory");
		return false;
	}
	return true;
}




void	PhysicsClientSharedMemory::processServerStatus()
{
	btAssert(m_data->m_testBlock1);

	if (m_data->m_testBlock1->m_numServerCommands> m_data->m_testBlock1->m_numProcessedServerCommands)
	{
		btAssert(m_data->m_testBlock1->m_numServerCommands==m_data->m_testBlock1->m_numProcessedServerCommands+1);
        
		const SharedMemoryCommand& serverCmd =m_data->m_testBlock1->m_serverCommands[0];
            
		//consume the command
		switch (serverCmd.m_type)
		{

			case CMD_URDF_LOADING_COMPLETED:
			{
				m_data->m_serverLoadUrdfOK = true;
				b3Printf("Server loading the URDF OK\n");

				if (serverCmd.m_dataStreamArguments.m_streamChunkLength>0)
					{
						bParse::btBulletFile* bf = new bParse::btBulletFile(this->m_data->m_testBlock1->m_bulletStreamDataServerToClient,serverCmd.m_dataStreamArguments.m_streamChunkLength);
						bf->setFileDNAisMemoryDNA();
						bf->parse(false);
						for (int i=0;i<bf->m_multiBodies.size();i++)
						{
							int flag = bf->getFlags();

							if ((flag&bParse::FD_DOUBLE_PRECISION)!=0)
							{
								Bullet::btMultiBodyDoubleData* mb = (Bullet::btMultiBodyDoubleData*)bf->m_multiBodies[i];
								if (mb->m_baseName)
								{
									b3Printf("mb->m_baseName = %s\n",mb->m_baseName);
								}
								for (int link=0;link<mb->m_numLinks;link++)
								{
									if (mb->m_links[link].m_linkName)
									{
										b3Printf("mb->m_links[%d].m_linkName = %s\n",link,mb->m_links[link].m_linkName);
									}
									if (mb->m_links[link].m_jointName)
									{
										b3Printf("mb->m_links[%d].m_jointName = %s\n",link,mb->m_links[link].m_jointName);
									}
								}
							} else
							{
								Bullet::btMultiBodyFloatData* mb = (Bullet::btMultiBodyFloatData*) bf->m_multiBodies[i];
								if (mb->m_baseName)
								{
									b3Printf("mb->m_baseName = %s\n",mb->m_baseName);
								}
								for (int link=0;link<mb->m_numLinks;link++)
								{
									if (mb->m_links[link].m_linkName)
									{
										b3Printf("mb->m_links[%d].m_linkName = %s\n",link,mb->m_links[link].m_linkName);
									}
									b3Printf("link [%d] type = %d",link, mb->m_links[link].m_jointType);
									if (mb->m_links[link].m_jointName)
									{
										b3Printf("mb->m_links[%d].m_jointName = %s\n",link,mb->m_links[link].m_jointName);
									}
								}
							}
						}
						printf("ok!\n");
					}
				break;
			}
			case CMD_DESIRED_STATE_RECEIVED_COMPLETED:
                {
                    break;
                }
			case CMD_STEP_FORWARD_SIMULATION_COMPLETED:
			{
				break;
			}
			case CMD_URDF_LOADING_FAILED:
			{
				b3Printf("Server failed loading the URDF...\n");
				m_data->m_serverLoadUrdfOK = false;
				break;
			}

			case CMD_BULLET_DATA_STREAM_RECEIVED_COMPLETED:
				{
					b3Printf("Server received bullet data stream OK\n");

					


					break;
				}
			case CMD_BULLET_DATA_STREAM_RECEIVED_FAILED:
				{
					b3Printf("Server failed receiving bullet data stream\n");

					break;
				}
                    

			case CMD_ACTUAL_STATE_UPDATE_COMPLETED:
				{
					b3Printf("Received actual state\n");
						
					int numQ = m_data->m_testBlock1->m_serverCommands[0].m_sendActualStateArgs.m_numDegreeOfFreedomQ;
					int numU = m_data->m_testBlock1->m_serverCommands[0].m_sendActualStateArgs.m_numDegreeOfFreedomU;
					b3Printf("size Q = %d, size U = %d\n", numQ,numU);
					char msg[1024];

					sprintf(msg,"Q=[");
						
					for (int i=0;i<numQ;i++)
					{
						if (i<numQ-1)
						{
							sprintf(msg,"%s%f,",msg,m_data->m_testBlock1->m_actualStateQ[i]);
						} else
						{
							sprintf(msg,"%s%f",msg,m_data->m_testBlock1->m_actualStateQ[i]);
						}
					}
					sprintf(msg,"%s]",msg);
						
					b3Printf(msg);
					b3Printf("\n");
					break;
				}
			default:
			{
				b3Error("Unknown server command\n");
				btAssert(0);
			}
		};
			
			
		m_data->m_testBlock1->m_numProcessedServerCommands++;
		//we don't have more than 1 command outstanding (in total, either server or client)
		btAssert(m_data->m_testBlock1->m_numProcessedServerCommands == m_data->m_testBlock1->m_numServerCommands);

		if (m_data->m_testBlock1->m_numServerCommands == m_data->m_testBlock1->m_numProcessedServerCommands)
		{
			m_data->m_waitingForServer = false;
		} else
		{
			m_data->m_waitingForServer = true;
		}
	}
}

bool PhysicsClientSharedMemory::canSubmitCommand() const
{
	return (m_data->m_isConnected && !m_data->m_waitingForServer);
}

bool	PhysicsClientSharedMemory::submitClientCommand(const SharedMemoryCommand& command)
{
	if (!m_data->m_waitingForServer)
		{
			//process command
			{
				m_data->m_waitingForServer = true;

				switch (command.m_type)
				{
				    
				case CMD_LOAD_URDF:
					{
						if (!m_data->m_serverLoadUrdfOK)
						{
							m_data->m_testBlock1->m_clientCommands[0].m_type =CMD_LOAD_URDF;
							sprintf(m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_urdfFileName,"r2d2.urdf");
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_initialPosition[0] = 0.0;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_initialPosition[1] = 0.0;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_initialPosition[2] = 0.0;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_initialOrientation[0] = 0.0;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_initialOrientation[1] = 0.0;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_initialOrientation[2] = 0.0;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_initialOrientation[3] = 1.0;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_useFixedBase = false;
							m_data->m_testBlock1->m_clientCommands[0].m_urdfArguments.m_useMultiBody = true;

							m_data->m_testBlock1->m_numClientCommands++;
							b3Printf("Client created CMD_LOAD_URDF\n");
						} else
						{
							b3Warning("Server already loaded URDF, no client command submitted\n");
						}
						break;
					}
				case CMD_CREATE_BOX_COLLISION_SHAPE:
					{
						if (m_data->m_serverLoadUrdfOK)
						{
							b3Printf("Requesting create box collision shape\n");
							m_data->m_testBlock1->m_clientCommands[0].m_type =CMD_CREATE_BOX_COLLISION_SHAPE;
							m_data->m_testBlock1->m_numClientCommands++;
						} else
						{
							b3Warning("No URDF loaded\n");
						}
						break;
					}
				case CMD_SEND_BULLET_DATA_STREAM:
					{
						b3Printf("Sending a Bullet Data Stream\n");
						///The idea is to pass a stream of chunks from client to server
						///over shared memory. The server will process it
						///Initially we will just copy an entire .bullet file into shared
						///memory but we can also send individual chunks one at a time
						///so it becomes a streaming solution
						///In addition, we can make a separate API to create those chunks
						///if needed, instead of using a 3d modeler or the Bullet SDK btSerializer
						
						char relativeFileName[1024];
						const char* fileName = "slope.bullet";
						bool fileFound = b3ResourcePath::findResourcePath(fileName,relativeFileName,1024);
						if (fileFound)
						{
							FILE *fp = fopen(relativeFileName, "rb");
							if (fp)
							{
								fseek(fp, 0L, SEEK_END);
								int mFileLen = ftell(fp);
								fseek(fp, 0L, SEEK_SET);
								if (mFileLen<SHARED_MEMORY_MAX_STREAM_CHUNK_SIZE)
								{
								
									fread(m_data->m_testBlock1->m_bulletStreamDataClientToServer, mFileLen, 1, fp);

									fclose(fp);

									m_data->m_testBlock1->m_clientCommands[0].m_type =CMD_SEND_BULLET_DATA_STREAM;
									m_data->m_testBlock1->m_clientCommands[0].m_dataStreamArguments.m_streamChunkLength = mFileLen;
									m_data->m_testBlock1->m_numClientCommands++;
									b3Printf("Send bullet data stream command\n");
								} else
								{
									b3Warning("Bullet file size (%d) exceeds of streaming memory chunk size (%d)\n", mFileLen,SHARED_MEMORY_MAX_STREAM_CHUNK_SIZE);
								}
							} else
							{
								b3Warning("Cannot open file %s\n", relativeFileName);
							}
						} else
						{
							b3Warning("Cannot find file %s\n", fileName);
						}
						
						break;
					}
				case CMD_REQUEST_ACTUAL_STATE:
					{
						if (m_data->m_serverLoadUrdfOK)
						{
							b3Printf("Requesting actual state\n");
							m_data->m_testBlock1->m_clientCommands[0].m_type =CMD_REQUEST_ACTUAL_STATE;
							m_data->m_testBlock1->m_numClientCommands++;

						} else
						{
							b3Warning("No URDF loaded\n");
						}
						break;
					}
                case CMD_SEND_DESIRED_STATE:
                    {
                        if (m_data->m_serverLoadUrdfOK)
						{
							b3Printf("Sending desired state (pos, vel, torque)\n");
							m_data->m_testBlock1->m_clientCommands[0].m_type =CMD_SEND_DESIRED_STATE;
							//todo: expose a drop box in the GUI for this
							int controlMode = CONTROL_MODE_VELOCITY;//CONTROL_MODE_TORQUE;

							switch (controlMode)
							{
							case CONTROL_MODE_VELOCITY:
								{
									m_data->m_testBlock1->m_clientCommands[0].m_sendDesiredStateCommandArgument.m_controlMode = CONTROL_MODE_VELOCITY;
									for (int i=0;i<MAX_DEGREE_OF_FREEDOM;i++)
									{
										m_data->m_testBlock1->m_desiredStateQdot[i] = 1;
										m_data->m_testBlock1->m_desiredStateForceTorque[i] = 100;
									}
									break;
								}
							case CONTROL_MODE_TORQUE:
								{
									m_data->m_testBlock1->m_clientCommands[0].m_sendDesiredStateCommandArgument.m_controlMode = CONTROL_MODE_TORQUE;
									for (int i=0;i<MAX_DEGREE_OF_FREEDOM;i++)
									{
										m_data->m_testBlock1->m_desiredStateForceTorque[i] = 100;
									}
									break;
								}
							default:
								{
									b3Printf("Unknown control mode in client CMD_SEND_DESIRED_STATE");
									btAssert(0);
								}
							}
							
							m_data->m_testBlock1->m_numClientCommands++;

						} else
						{
							b3Warning("Cannot send CMD_SEND_DESIRED_STATE, no URDF loaded\n");
						}
                        break;
                    }
				case CMD_STEP_FORWARD_SIMULATION:
					{
						if (m_data->m_serverLoadUrdfOK)
						{
						
							m_data->m_testBlock1->m_clientCommands[0].m_type =CMD_STEP_FORWARD_SIMULATION;
							m_data->m_testBlock1->m_clientCommands[0].m_stepSimulationArguments.m_deltaTimeInSeconds = 1./60.;
							m_data->m_testBlock1->m_numClientCommands++;
							b3Printf("client created CMD_STEP_FORWARD_SIMULATION %d\n", m_data->m_counter++);
						} else
						{
							b3Warning("No URDF loaded yet, no client CMD_STEP_FORWARD_SIMULATION submitted\n");
						}
						break;
					}
				case CMD_SHUTDOWN:
					{
						
						m_data->m_testBlock1->m_clientCommands[0].m_type =CMD_SHUTDOWN;
						m_data->m_testBlock1->m_numClientCommands++;
						m_data->m_serverLoadUrdfOK = false;
						b3Printf("client created CMD_SHUTDOWN\n");
						break;
					}
				default:
					{
						b3Error("unknown command requested\n");
					}
				}
			}
		}
		
		return true;
}

