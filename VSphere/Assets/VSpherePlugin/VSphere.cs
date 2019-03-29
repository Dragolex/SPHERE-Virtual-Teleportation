using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;

using Microsoft.Win32.SafeHandles;

/*
 * This file uses the VSphere.
 * 
 * It loads the external DLL into unity and initializes its functions.
 * Additionally it handles the extraction of the 3D model
 */
 
public class VSphere : MonoBehaviour
{
    // name of the plugin
    const String VSphereLibrary = "VSpherePlugin";


    // DLL functions (For their accurate purpose and arguments see the source files of the DLL)

    [DllImport(VSphereLibrary)]
    private static extern bool PrepareSphere(bool open_console, int recorder_frame_duration_ms);
    [DllImport(VSphereLibrary)]
    private static extern int GetInternalData(string data_element);
    [DllImport(VSphereLibrary)]
    private static extern bool SetInternalData(string data_element);
    [DllImport(VSphereLibrary)]
    private static extern int ConfigureCamera(int hardware_index, int hardware_channel, int focus_value, int location_x, int location_y, int location_z, int offset_x, int offset_y, int offset_z);
    [DllImport(VSphereLibrary)]
    private static extern bool ConfigureRecordHandler(int configured_camera_index, string file_path, int type);
    [DllImport(VSphereLibrary)]
    private static extern bool StartSphere(int preview_window_variant, int preview_type);
    [DllImport(VSphereLibrary)]
    private static extern void QuitSphere();
    [DllImport(VSphereLibrary)]
    private static extern bool CheckNewModel();
    [DllImport(VSphereLibrary)]
    private static extern bool WaitForNextSphereFrame();

    // Texture preparation functions
    [DllImport(VSphereLibrary)]
    private static extern int GetRequiredTextureWidth();
    [DllImport(VSphereLibrary)]
    private static extern int GetRequiredTextureHeight();
    [DllImport(VSphereLibrary)]
    private static extern int ProvideTextureHandle(System.IntPtr textureHandle);

    // Model retrieving functions
    [DllImport(VSphereLibrary, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    static unsafe extern bool StartRetrievingModel(out int* quadsData, out int quadsCount);
    [DllImport(VSphereLibrary, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    static unsafe extern bool EndRetrievingModel();



    // Settings

    bool paused = false; // Stopped updating or not
    bool fullRays = false; // Whether the entire rays of the model shall be visible (true) or only the actual surface of the model 8false)
    bool previewEnabled = true; // Whether the preview window is visible
    int previewType = 5; // Which preview type to start with. Possibilities:
                         /*
                               Values:
							   0 "Preview disabled",
							   1 "Original live image",
							   2 "Dynamic background reference",
							   3 "Live image without background (by RGB)",
							   4 "Binary mask",
							   5 "In or Out mask grid",
							   6 "Contours mask",
							   7 "Contour keypoints",
							   8 "Contour segments",
							   9 "Contour segments as overlay"
						   */
    int previewWindowVariant = 2; // Type of the preview window: 0 = disabled; 1 = separated; 2 = combined;
    int previewWindowOrderOffset = 0; // When the combiend preview window is used, this variable determiens which camera is on full display
    bool calcTime = false; // Whether to calculate how long the transferation and building of a mdoel frame takes

    bool settingsApplied = false;


    void Start()
    {
        // Creating the root path where the VSphere, VSphere_DLL and Records folder can be found
        string rootPath = Application.dataPath + "/../../";
        rootPath = System.IO.Path.GetFullPath(rootPath);
        string extRootPath = rootPath.Replace("/", "\\");


        Debug.Log("Preparing VSphere");
        // Prepare the sphere with enabled console (true) and 33ms standard delay between frames (this delay is only used when frames are read from files and not directly streamed)
        PrepareSphere(true, 33);
        // Initialize the cameras (non-external function)
        InitializeCameras(extRootPath + "Records\\", "TestRecordB");
        // Initialize texture related things (non-external function)
        InitializeSphereTexture();
        // Start the sphere with the given settings
        StartSphere(previewEnabled ? previewWindowVariant : 0, previewType);
        // Initialize the submeshes in Unity (non-external function)
        InitializeSubmesh();


        Debug.Log("VSphere running.");
    }


    /*
     * Initializes sample cameras and the records through the given path and filename
     * */
    void InitializeCameras(string recordsRootPath, string fileName)
    {
        // Prepare cameras (for mroe detailed arguments see the DLL)
        Debug.Log("Preparing Cameras");
        
        int camA = ConfigureCamera(0, -1, 5,
            1, 1, 320, // Root coordinate of the camera in space. It's orientation is always towards 0,0,0
            45, 0, 0); // Additional offset of the camera (after the orientation towards 0,0,0 has been computed)
        int camB = ConfigureCamera(1, -1, 5,
            1, 480, 1,
            135, 0, 0);

        // Prepare the recording of the cameras or their playback
        int record_type = 2; // 1: Recording; 2: Playing;
        Debug.Log("Preparing Record configurations. Type: " + record_type);
        ConfigureRecordHandler(camA, recordsRootPath + fileName + camA, record_type);
        ConfigureRecordHandler(camB, recordsRootPath + fileName + camB, record_type);
    }



    float texWidth, texHeight;
    Material modelMaterial;

    /*
     * Prepares the texture which is used for rendering the model.
     * 
     * The system works as follows:
     * A texture of the appropiate size is created on Unity side.
     * All cameras are on the same the texture.
     * Its "native pointer" is retrieved and transfered to the DLL.
     * The DLL can swap the texture image any time and does so whenever a frame is decoded right bevore providing the new model.
     * The texture with the constant index is applied to a new material which can be used for the model.
     * */
    void InitializeSphereTexture()
    {
        // Prepare the texture sizes
        Debug.Log("Preparing texture.");
        texWidth = GetRequiredTextureWidth();
        texHeight = GetRequiredTextureHeight();

        //C reate new texture
        Texture2D mainTexture = new Texture2D((int)texWidth, (int)texHeight, TextureFormat.ARGB32, false);
        mainTexture.filterMode = FilterMode.Point;
        mainTexture.Apply(); // Ensure that texture is in GPU


        // Provide the texture pointer to the plugin
        Debug.Log("Applying texture with width " + texWidth + " and height " + texHeight + ".");
        ProvideTextureHandle(mainTexture.GetNativeTexturePtr());


        // Create the new material which uses this texture
        Debug.Log("Creating material.");
        modelMaterial = new Material(Shader.Find("Standard"));
        //modelMaterial = new Material(Shader.Find("Sprites/Default")); // The default sprite shader allows transparency if required but therefore not shadows
        modelMaterial.mainTexture = mainTexture;


        //Set the texture to that textureTestPlane which simply displays the cameras int he 3D environment
        if (GameObject.Find("textureTestPlane") != null)
            GameObject.Find("textureTestPlane").GetComponent<Renderer>().material.mainTexture = mainTexture;
    }


    int maxSphereSubObjects = 12;
    GameObject[] sphereSubObject;
    Mesh[] sphereMesh;
    MeshRenderer[] sphereRenderer;

    /*
     * Unity's rendering has a limit of ~2^16 coordinates per mesh. That is a lot but might be exceeded.
     * Therefore this system automatically splits the data into multiple meshes all using the same material/texture.
     * */
    void InitializeSubmesh()
    {
        Debug.Log("Preparing mesh sub objects");

        sphereSubObject = new GameObject[maxSphereSubObjects];
        sphereMesh = new Mesh[maxSphereSubObjects];
        sphereRenderer = new MeshRenderer[maxSphereSubObjects];

        for (int s = 0; s < maxSphereSubObjects; ++s)
        {
            sphereSubObject[s] = new GameObject();
            sphereMesh[s] = new Mesh();

            (sphereSubObject[s].AddComponent<MeshFilter>()).mesh = sphereMesh[s];

            sphereRenderer[s] = sphereSubObject[s].AddComponent<MeshRenderer>();
            sphereRenderer[s].material = modelMaterial;

            sphereRenderer[s].shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.On;
            sphereMesh[s].MarkDynamic();
        }
    }



    
    float maxVerticesPerSubSphere = 64000;
    int quadCount;
    System.Diagnostics.Stopwatch stopwatch = null;

    /*
     * This is the actual update function which is called once every time the world is updated.
     * Since the number of camera frames is unlikely to exceed the physics FPS, this simple update event is used.
     * It would be possible to transfer this to the rendering thread which is usually updates with a higher rate.
     * 
     * The system works by providing variables to the DLL and retrieve the pointer of a data array made of int.
     * This array contains the data of the new model which is formed by quads (four points on one plane).
     * The following values are used for eevry quad:
     * Value 1-3: X, Y and Z coordinate of the first point multiplicated by 1000 (to achieve accuray but not require to transfer floats)
     * Value 4-6: X, Y and Z coordinate of the second point multiplicated by 1000
     * Value 7-9: X, Y and Z coordinate of the third point multiplicated by 1000
     * Value 10-12: X, Y and Z coordinate of the fourth point multiplicated by 1000
     * Value 13-20: U and V coordinates for every point in pairs of two.
     * */
    unsafe void Update()
    {
        if (paused) // Return if paused
            return;

        if (!CheckNewModel()) // Return if no new model ready
            return;

        if (!settingsApplied)
        {
            ApplySettings(); // Because the Sphere inside the dll runs in another Thread, exacute this only after the system has been set up and the first frame computed.
            settingsApplied = true;
        }


        if (calcTime)
        {
            stopwatch = new System.Diagnostics.Stopwatch();
            stopwatch.Reset();
            stopwatch.Start();
        }


        int* quadsData;
        int currentDataPos = 0;

        // Start retrieving the model. Pointers are used to hold the data.
        // This method is faster than "marshalling" because it does not require to copy the entire array of data.
        StartRetrievingModel(out quadsData, out quadCount);

        int vertCount = quadCount * 4;
        // Determine how many objects should be used.
        int activeSphereObjects = Mathf.CeilToInt(vertCount / maxVerticesPerSubSphere); // because number of vertices per mesh is limited in Unity to aproximates 64 000

        Debug.Log("Displaying " + quadCount + " quads in " + activeSphereObjects + " sub objects.");


        // Clear all sub objects
        for (int s = 0; s < maxSphereSubObjects; ++s)
            sphereMesh[s].Clear();


        // Preparing memmory for the model (TODO: This might be possible to be optimized)
        Vector3[][] vertices = new Vector3[activeSphereObjects][];
        Vector2[][] uv = new Vector2[activeSphereObjects][];


        int u = 0;
        for (int s = 0; s < activeSphereObjects; ++s)
        {
            int currentCount = (int)Math.Min(maxVerticesPerSubSphere, vertCount - u);

            vertices[s] = new Vector3[currentCount];
            uv[s] = new Vector2[currentCount];


            const float quotient = 1000;

            int i = 0;
            for (int w = 0; w < currentCount; w += 4)
            {
                // Read the quads and the UV coordinates

                vertices[s][i] = new Vector3(quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient);
                vertices[s][i + 1] = new Vector3(quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient);
                vertices[s][i + 2] = new Vector3(quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient);
                vertices[s][i + 3] = new Vector3(quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient, quadsData[currentDataPos++] / quotient);

                uv[s][i] = new Vector2(quadsData[currentDataPos++] / texWidth, quadsData[currentDataPos++] / texHeight);
                uv[s][i + 1] = new Vector2(quadsData[currentDataPos++] / texWidth, quadsData[currentDataPos++] / texHeight);
                uv[s][i + 2] = new Vector2(quadsData[currentDataPos++] / texWidth, quadsData[currentDataPos++] / texHeight);
                uv[s][i + 3] = new Vector2(quadsData[currentDataPos++] / texWidth, quadsData[currentDataPos++] / texHeight);

                i += 4;
                u += 4;
            }
        }

        // Finish retrieving - unlocks the memmory and allows the next frame (which may have been processed in the meantime) to write its data.
        EndRetrievingModel();


        // The next code computes the actual indexes of the triangles because Unity cannot work directly with quads
        u = 0;
        for (int s = 0; s < activeSphereObjects; ++s)
        {
            int currentCount = (int)Math.Min(maxVerticesPerSubSphere, vertCount - u);

            int[] triangles = new int[currentCount * 3];
            Vector3[] normals = new Vector3[currentCount];


            int j = 0; int i = 0;
            for (int w = 0; w < currentCount; w += 4)
            {
                triangles[j] = i;
                triangles[j + 1] = i + 1;
                triangles[j + 2] = i + 2;

                triangles[j + 3] = i + 1;
                triangles[j + 4] = i + 2;
                triangles[j + 5] = i + 3;


                triangles[j + 6] = i + 2;
                triangles[j + 7] = i + 1;
                triangles[j + 8] = i;

                triangles[j + 9] = i + 3;
                triangles[j + 10] = i + 2;
                triangles[j + 11] = i + 1;


                Vector3 N = Vector3.Cross(vertices[s][i] - vertices[s][i + 1], vertices[s][i + 1] - vertices[s][i + 2]);
                N.Normalize();

                normals[i] = N;
                normals[i + 1] = N;
                normals[i + 2] = N;
                normals[i + 3] = N;

                i += 4;
                j += 12;
                u += 4;
            }

            // Applying all data to the mesh
            sphereMesh[s].vertices = vertices[s];
            sphereMesh[s].triangles = triangles;
            sphereMesh[s].normals = normals;
            sphereMesh[s].uv = uv[s];

            sphereMesh[s].RecalculateBounds();
        }


        if (calcTime)
        {
            stopwatch.Stop();
            Debug.Log("Time for retrieving the frame: " + stopwatch.ElapsedTicks);
        }

    }


    void ApplySettings()
    {
        FullRays = fullRays; // Whether the entire rays of the model shall be visible (true) or only the actual surface of the model 8false)
        PreviewEnabled = previewEnabled;
        PreviewType = previewType;
        PreviewWindowVariant = previewWindowVariant;
        PreviewWindowOrderOffset = previewWindowOrderOffset;
    }

    void OnApplicationQuit()
    {
        QuitSphere();
    }


    public int getPolygonCount()
    {
        return ((quadCount*4)/3);
    }



    // The following functions are required for the GUI settings

    public bool FullRays
    {
        get
        {
            return fullRays;
        }

        set
        {
            SetInternalData(value ? "Full rays: true" : "Full rays: false");
            fullRays = value;
        }
    }

    public bool Paused
    {
        get
        {
            return paused;
        }

        set
        {
            paused = value;
        }
    }

    public int PreviewType
    {
        get
        {
            return previewType;
        }

        set
        {
            previewType = value;
            SetInternalData("Preview type: " + previewType.ToString());
        }
    }

    public int PreviewWindowOrderOffset
    {
        get
        {
            return previewWindowOrderOffset;
        }

        set
        {
            previewWindowOrderOffset = value;
            SetInternalData("Preview window order offset: " + previewWindowOrderOffset.ToString());
        }
    }

    public bool PreviewEnabled
    {
        get
        {
            return previewEnabled;
        }

        set
        {
            previewEnabled = value;
            if (!previewEnabled)
                SetInternalData("Preview window variant: 0");
            else
                SetInternalData("Preview window variant: " + PreviewWindowVariant.ToString());
        }
    }

    public int PreviewWindowVariant
    {
        get
        {
            return previewWindowVariant;
        }

        set
        {
            previewWindowVariant = value;
            SetInternalData("Preview window variant: " + PreviewWindowVariant.ToString());
        }
    }

    internal void nextPreviewType()
    {
        SetInternalData("Next preview type");
        previewType = GetInternalData("Preview type");
    }

    internal void lastPreviewType()
    {
        SetInternalData("Last preview type");
        previewType = GetInternalData("Preview type");
    }

    internal void nextPreviewWindowOrderOffset()
    {
        SetInternalData("Next preview window order offset");
        previewWindowOrderOffset = GetInternalData("Preview window order offset");
    }

    internal void lastPreviewWindowOrderOffset()
    {
        SetInternalData("Last preview window order offset");
        previewWindowOrderOffset = GetInternalData("Preview window order offset");
    }

}