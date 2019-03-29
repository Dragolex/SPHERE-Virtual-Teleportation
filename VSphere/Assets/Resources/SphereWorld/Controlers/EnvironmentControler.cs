using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using UnityEngine.SceneManagement;

/*
 */
 
public class EnvironmentControler : MonoBehaviour {

    public List<FloorLevel> floorLevels = new List<FloorLevel>();
    public GameObject wallLeft, wallBack, wallRight, wallFront;


    int testfieldHeight;


    Vector3 standardFloorScale;
    Vector2 standardFloorTextureScale;

    float sceneChangeTimer = 0;
    float sceneChangeDuration = 2f;
    string sceneChangeTo = "";


    Texture2D fadeTexture;
    Rect screenRect;

    public bool justMovedToWatchNew = false;

    bool renderPhTemp, renderMemTemp;


    // Initializes various environment variables and elements
	void Awake () {

        // Initialise static objects
        Global.Init();
        GlobalSettings.Init();


        // Screen fade
        sceneChangeTimer = sceneChangeDuration;
        screenRect = new Rect(0, 0, Screen.width, Screen.height);
        fadeTexture = new Texture2D(1, 1);


            // Create Floors
            standardFloorScale = Global.BaseQuad.transform.localScale;

            if (Global.BaseQuad == null)
                Global.ShowMessage("baseQuad is empty");

            if (Global.BaseQuad.GetComponent<Renderer>() == null)
                Global.ShowMessage("baseQuad renderer is empty");

            if (Global.BaseQuad.GetComponent<Renderer>().sharedMaterial == null)
                Global.ShowMessage("baseQuad renderer sharedMaterial is empty");


            standardFloorTextureScale = Global.BaseQuad.GetComponent<Renderer>().sharedMaterial.mainTextureScale;


        bool create_quads = false;

        if (create_quads)
        {
            // Local settings
            int numOfFloor = 3;
            int floorDist = 12;
            float floorAlpha = 0.1f;
            float wallAlpha = 0.05f;


            testfieldHeight = numOfFloor * floorDist;
            Color col, midcol = Color.white;


            GameObject floor;


            for (int pos = -numOfFloor * floorDist; pos <= numOfFloor * floorDist; pos += floorDist)
            {
                if (pos < 0)
                    col = Color.Lerp(Color.white, Color.red, (float)((-pos) / (float)(testfieldHeight)) * 1.6f);
                else if (pos > 0)
                    col = Color.Lerp(Color.white, GlobalSettings.focusColor, (float)((pos) / (float)(testfieldHeight)) * 1.6f);
                else col = midcol;
                col.a = floorAlpha;

                floor = createQuad(new Vector3(0, pos, 0), Quaternion.Euler(new Vector3(90, 0, 0)), col, standardFloorScale);

                col.a = 1;
                floorLevels.Add(new FloorLevel(floor, pos, col));
                // *Mathf.Min(1f, ((float)(numOfFloor * floorDist) / (float)(Mathf.Abs(pos))));
            }


            // Walls

            wallBack = createQuad(new Vector3(0, 0, -floorLevels[1].floor.transform.localScale.y / 2), Quaternion.Euler(new Vector3(0, 0, 0)), new Color(1, 1, 1, wallAlpha), new Vector3(standardFloorScale.x, 2 * testfieldHeight, 1), new Vector2(1, 1));
            wallFront = createQuad(new Vector3(0, 0, floorLevels[1].floor.transform.localScale.y / 2), Quaternion.Euler(new Vector3(0, 0, 0)), new Color(1, 1, 1, wallAlpha), new Vector3(standardFloorScale.x, 2 * testfieldHeight, 1), new Vector2(1, 1));
            wallLeft = createQuad(new Vector3(-standardFloorScale.x / 2, 0, 0), Quaternion.Euler(new Vector3(0, 90, 0)), new Color(1, 1, 1, wallAlpha), new Vector3(floorLevels[1].floor.transform.localScale.y, 2 * testfieldHeight, 1), new Vector2(1, 1));
            wallRight = createQuad(new Vector3(+standardFloorScale.x / 2, 0, 0), Quaternion.Euler(new Vector3(0, 90, 0)), new Color(1, 1, 1, wallAlpha), new Vector3(floorLevels[1].floor.transform.localScale.y, 2 * testfieldHeight, 1), new Vector2(1, 1));


            ResizeFloors(4);
        }

        Debug.Log("Environment Started");
	}


    void Start()
    {

    }





    GameObject createQuad(Vector3 position, Quaternion rotation, Color col, Vector3 scale)
    {
        GameObject floor = (GameObject)Instantiate(Global.BaseQuad, position, rotation);
        floor.GetComponent<Renderer>().material.color = col;
        foreach (Transform backFace in floor.transform)
            backFace.gameObject.GetComponent<Renderer>().material.color = col;
        floor.transform.localScale = scale;

        return (floor);
    }
    GameObject createQuad(Vector3 position, Quaternion rotation, Color col, Vector3 scale, Vector2 texScale)
    {
        GameObject floor = createQuad(position, rotation, col, scale);

        floor.GetComponent<Renderer>().material.mainTextureScale = texScale;
        foreach (Transform backFace in floor.transform)
            backFace.gameObject.GetComponent<Renderer>().material.mainTextureScale = texScale;

        return (floor);
    }

    void modifyQuad(GameObject floor, Vector3 position)
    {
        floor.transform.position = position;
    }
    void modifyQuad(GameObject floor, Vector3 position, Vector3 scale)
    {
        floor.transform.position = position;
        floor.transform.localScale = scale;
    }

    void modifyQuad(GameObject floor, Vector3 position, Vector3 scale, Vector3 texScale)
    {
        floor.transform.position = position;
        floor.transform.localScale = scale;

        floor.GetComponent<Renderer>().material.mainTextureScale = texScale;
        foreach (Transform backFace in floor.transform)
            backFace.gameObject.GetComponent<Renderer>().material.mainTextureScale = texScale;
    }

    public void SetQuadRender(GameObject floor, bool render)
    {
        floor.GetComponent<Renderer>().enabled = render;
        foreach (Transform backFace in floor.transform)
            backFace.gameObject.GetComponent<Renderer>().enabled = render;
    }



    public void ResizeFloors(float scale)
    {
        foreach (FloorLevel floorLevel in floorLevels)
        {
            //if (floorLevel.level == 0)
              //  floorLevel.floor.transform.localScale = standardFloorScale * scale * 1.1f;
            //else
            floorLevel.floor.transform.localScale = standardFloorScale * scale;
            floorLevel.floor.GetComponent<Renderer>().material.mainTextureScale = standardFloorTextureScale * Mathf.Round(scale);
            foreach (Transform backFace in floorLevel.floor.transform)
                backFace.gameObject.GetComponent<Renderer>().material.mainTextureScale = standardFloorTextureScale * Mathf.Round(scale);
        }

        Vector3 currentFloorScale = floorLevels[1].floor.transform.localScale;


        modifyQuad(wallBack, new Vector3(0, 0, -currentFloorScale.y / 2), new Vector3(currentFloorScale.x, 2 * testfieldHeight, 1), new Vector2(Mathf.Round(scale), 1));
        modifyQuad(wallFront, new Vector3(0, 0, currentFloorScale.y / 2), new Vector3(currentFloorScale.x, 2 * testfieldHeight, 1), new Vector2(Mathf.Round(scale), 1));
        modifyQuad(wallLeft, new Vector3(-currentFloorScale.x / 2, 0, 0), new Vector3(currentFloorScale.y, 2 * testfieldHeight, 1), new Vector2(Mathf.Round(scale), 1));
        modifyQuad(wallRight, new Vector3(+currentFloorScale.x / 2, 0, 0), new Vector3(currentFloorScale.y, 2 * testfieldHeight, 1), new Vector2(Mathf.Round(scale), 1));
    }



    // Frame Update
    void Update () {
   
        // Scene Blending
        if (sceneChangeTimer > 0)
        {
            sceneChangeTimer -= Time.deltaTime;

            if (sceneChangeTimer <= 0.15)
            {
                if (!sceneChangeTo.Equals(""))
                    SceneManager.LoadScene(sceneChangeTo);
            }
        }


        // Scene Change
        if (Input.GetKeyDown(KeyCode.F))
        {
            if (Global.GetCurrentScene() == Global.mainAIScene)
                SwitchScene("CliffSimulation");
        }


	}


    public void SwitchScene(string sceneName)
    {
        sceneChangeTimer = sceneChangeDuration;
        sceneChangeTo = sceneName;
    }


    // See GUI event of GUIScript
    public void DrawSceneChangeGUI()
    {
        if (sceneChangeTimer > 0)
        {
            if (sceneChangeTo.Equals(""))
                GUI.color = Color.Lerp(Color.clear, Color.black, (sceneChangeTimer * 1.25f) / sceneChangeDuration);
            else GUI.color = Color.Lerp(Color.black, Color.clear, (sceneChangeTimer * 1.25f) / sceneChangeDuration);

            GUI.DrawTexture(screenRect, fadeTexture, ScaleMode.StretchToFill);
            GUI.DrawTexture(screenRect, fadeTexture, ScaleMode.StretchToFill);
        }
    }


}


// DATASTRUCTURE for FLOORLEVELS ///////////////////////////////

public struct FloorLevel
{
    public GameObject floor;
    public int level;
    public Color col;

    public FloorLevel(GameObject floor, int level, Color col)
    {
        this.floor = floor;
        this.level = level;
        this.col = col;
    }
}