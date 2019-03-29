using UnityEngine;
using System.Collections;


/*
    This script displays and handles the GUI.
 */
 
public class GUIControler : MonoBehaviour {

    // For buttons and text
    Rect btLineRect = new Rect(0, 0, 0, 0);
    int btLineDist;

    // textfields
    TextFieldScript titleTF, controlsTF, functionsTF, previewTF;


    // Whether the GUI functions have not been initialized yet
    bool initGUI = true;
    GUIStyle styleAlwaysActive;



    // For fps
    float currentFPS = 0;
    int currentFPSCounter = 0;
    float currentFPSSum = 0;


    // Other variables
    bool pressedPause = false;


    void Start () {

        // Some coordinates for the GUI
        int fy = 10;
        int fw = 400;
        int btx = 275;
        int fh = 25;
        int offs = 7;

        VSphere sphere = GameObject.Find("SphereControler").GetComponent<VSphere>();


        // Create the title textfield with displaying the FPS and running time
        titleTF = (TextFieldScript)Instantiate(Global.TextFieldObject).GetComponent(typeof(TextFieldScript));
        titleTF.InitializeTextField(10, fy, fw, fh, offs, 5, true, Color.cyan, 18, FontStyle.Bold);


        titleTF.AddTextLine("VR Sphere ");
        titleTF.AddAutoUpdateLine("Current render FPS: ", () => { return (currentFPS.ToString()); });
        titleTF.AddAutoUpdateLine("Total running time: ", () => { return (Time.realtimeSinceStartup.ToString("F2")); });
        titleTF.AddAutoUpdateLine("3D camera position: ", () => { return (Global.mainCameraScr.getPosition().ToString()); });
        titleTF.AddAutoUpdateLine("Current polygons: ", () => { return (sphere.getPolygonCount().ToString());});


        fy += 5 * fh + 3 * offs;
        controlsTF = (TextFieldScript)Instantiate(Global.TextFieldObject).GetComponent(typeof(TextFieldScript));
        controlsTF.InitializeTextField(10, fy, fw, fh, offs, 5);
        controlsTF.InitializedActiveLineSettings(btx, offs, () => { Global.mainCameraScr.doubleClick = 0; });
        controlsTF.AddTextLine("<color=#00ffffff>Controls</color>     ");
        controlsTF.AddTextLine("<Double-Click> screen for mouse control");
        controlsTF.AddTextLine("Free camera movement with <WASD> keys");
        controlsTF.AddTextLine("Camera zoom out with <Q>. In with <E>. Or <Scrollwheel>");
        controlsTF.AddTextLine("Loop through predefined points with <Spacebar>");



        fy += 5 * fh + 3 * offs;
        functionsTF = (TextFieldScript)Instantiate(Global.TextFieldObject).GetComponent(typeof(TextFieldScript));
        functionsTF.InitializeTextField(10, fy, fw, fh, offs, 3);
        functionsTF.InitializedActiveLineSettings(btx, offs, () => { Global.mainCameraScr.doubleClick = 0; });

        functionsTF.AddTextLine("<color=#00ffffff>Settings</color>     ");
        functionsTF.AddActiveLine("Rays: ", "Whether the whole rays should be drawn or the actual desired model.", "",
                                                 new string[] { "Object", "Full" }, new System.Action[] {()=>{ sphere.FullRays = false;},
                                                                                                         ()=>{ sphere.FullRays = true;} },
                                                                                                         () => { return (sphere.FullRays ? "Full" : "Object"); },
                                                                                                         KeyCode.U);

        functionsTF.AddActiveLine("Paused: ", "Model updating is paused or not.", "",
                                         new string[] { "Yes", "No" }, new System.Action[] {()=>{ sphere.Paused = true;},
                                                                                            ()=>{ sphere.Paused = false;} },
                                                                                            () => { return (sphere.Paused ? "Yes" : "No"); },
                                                                                            KeyCode.P);

        fy += 3 * fh + 3 * offs;
        previewTF = (TextFieldScript)Instantiate(Global.TextFieldObject).GetComponent(typeof(TextFieldScript));
        previewTF.InitializeTextField(10, fy, fw, fh, offs, 4);
        previewTF.InitializedActiveLineSettings(btx, offs, () => { Global.mainCameraScr.doubleClick = 0; });

        previewTF.AddTextLine("<color=#00ffffff>Preview</color>     ");

        /*
        previewTF.AddActiveLine("Preview Window: ", "The camera input preview is visible or not.", "",
                                         new string[] { "OFF", "ON" }, new System.Action[] {()=>{sphere.PreviewEnabled = false;},
                                                                                            ()=>{sphere.PreviewEnabled = true;} },
                                                                                            () => { return (sphere.PreviewEnabled ? "ON" : "OFF"); },
                                                                                            KeyCode.P);
        */

        previewTF.AddActiveLine("Preview Mode: ", "Type of preview mode.", "",
                                                 new string[] { "Last", "Next" }, new System.Action[] {()=>{sphere.nextPreviewType();},
                                                                                                       ()=>{sphere.lastPreviewType();} },
                                                                                                       () => { return (sphere.PreviewType.ToString()); },
                                                                                                       KeyCode.M, KeyCode.DownArrow, KeyCode.UpArrow);


        previewTF.AddActiveLine("Separated Windows: ", "The camera input preview is separated into multiple windows.", "",
                                         new string[] { "OFF", "ON" }, new System.Action[] {()=>{sphere.PreviewWindowVariant = 2;},
                                                                                            ()=>{sphere.PreviewWindowVariant = 1;} },
                                                                                            () => { return (sphere.PreviewWindowVariant == 1 ? "ON" : "OFF"); },
                                                                                            KeyCode.I);

        previewTF.AddActiveLine("Preview focus: ", "Which camera is in focus in the non-separated preivew window.", "",
                                                 new string[] { "Last", "Next" }, new System.Action[] {()=>{sphere.lastPreviewWindowOrderOffset();},
                                                                                                       ()=>{sphere.nextPreviewWindowOrderOffset();} },
                                                                                                       () => { return (sphere.PreviewWindowOrderOffset.ToString()); },
                                                                                                       KeyCode.F, KeyCode.UpArrow, KeyCode.DownArrow);

    }


    // Update is called once per frame
    void Update ()
    {

        currentFPSCounter++;
        currentFPSSum += (1.0f / Time.deltaTime);
        if (currentFPSCounter >= 15)
        {
            currentFPS = Mathf.Floor(currentFPSSum / currentFPSCounter);
            currentFPSSum = 0;
            currentFPSCounter = 0;
        }
	}



    void StartButtonLine(int offsx, int offsy, int sep, int height)
    {
        btLineRect.x = offsx;
        btLineRect.y = offsy;
        btLineRect.height = height;
        btLineDist = sep;
    }

    bool DrawInlineButton(int width, string str, bool sep)
    {
         return(DrawInlineButton(width, str, sep, false));
    }
    bool DrawInlineButton(int width, string str, bool sep, bool keptDown)
    {
        if (sep) btLineRect.x += btLineDist;
        btLineRect.width = width;

        bool ret;
        if (keptDown)
            ret = GUI.Button(btLineRect, str, styleAlwaysActive);
        else
        ret = GUI.Button(btLineRect, str, GUI.skin.button);

        btLineRect.x += width + btLineDist;

        if (ret) Global.mainCameraScr.doubleClick = 0; // prevent double click to trigger

        return(ret);
    }


    


    // GUI Draw
    void OnGUI()
    {
        if (initGUI)
        {
            // Modified GUI Styles

            GUI.skin.button.fontSize = 18;
            GUI.skin.button.fontStyle = FontStyle.Bold;

            GUI.skin.label.fontSize = 15;
            GUI.skin.label.fontStyle = FontStyle.Bold;


            styleAlwaysActive = new GUIStyle(GUI.skin.button);

            styleAlwaysActive.normal = styleAlwaysActive.hover;
            styleAlwaysActive.hover = styleAlwaysActive.hover;

            initGUI = false;
        }

        
        /*
        if ((GUI.Button(new Rect(10, additionalButtonsY, 133, 30), "Restart (R)")) || (Input.GetKeyDown(KeyCode.R)))
        {
            Global.mainCameraScr.doubleClick = 0;
            PopulationControler.Init();
            Global.envControlerScr.SwitchScene("MainScene");
        }

        if ((GUI.Button(new Rect(129 + 21, additionalButtonsY, 133, 30), "Quit (Esc)")) || ((Input.GetKeyDown(KeyCode.Escape)) && (GlobalSettings.tutorialState >= 110)))
        {
            Global.mainCameraScr.doubleClick = 0;
            Application.Quit();

            Global.ShowMessage("");
        }
        */

        if (Input.GetKeyDown(KeyCode.P))
        {
            if (!pressedPause)
            {
                if (Time.timeScale != 0) Time.timeScale = 0;
                else Time.timeScale = 1.5f;

                pressedPause = true;
            }
        }
        if (pressedPause)
        if (Input.GetKeyUp(KeyCode.P))
            pressedPause = false;

        /*
        if ((GUI.Button(new Rect(264 + 26, additionalButtonsY, 133, 30), "Cliffscene (F)")) || (Input.GetKeyDown(KeyCode.F)))
        {
            if (Global.GetCurrentScene() == Global.mainAIScene)
                Global.envControlerScr.SwitchScene("CliffSimulation");
            else if (Global.GetCurrentScene() == Global.cliffScene)
                Global.envControlerScr.SwitchScene("MainScene");
        }
        */


        Global.envControlerScr.DrawSceneChangeGUI();
    }

}
