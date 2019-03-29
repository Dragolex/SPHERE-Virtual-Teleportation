using UnityEngine;
using System.Collections;
using UnityEngine.SceneManagement;

/*
 * This class with static-only members serves globally used data for the game as well as a few functions.
 * For example references to important, unique scripts
 * 
 * Attention: The "Init" Function is not called automatically.
 */


public class Global : MonoBehaviour {

    // Important components/scripts
    static public EnvironmentControler envControlerScr;
    static public MainCameraScript mainCameraScr;
    static public GUIControler GUIControlerScr;


    // Resource references
    static public GameObject TextFieldObject = (GameObject)Resources.Load("SphereWorld/GUI/TextFieldObj");
    static public GameObject BaseQuad = (GameObject)Resources.Load("SphereWorld/Surroundings/BaseQuad");


    // Unity Scene names used
    static public Scene mainAIScene = SceneManager.GetSceneByName("MainScene");
    static public Scene currentScene = SceneManager.GetActiveScene();

    // A few other constants
    static public readonly Quaternion quatNull = new Quaternion(0, 0, 0, 1);
    static public readonly Quaternion quatRight = Quaternion.Euler(Vector3.up * 90);
    static public readonly Quaternion quatLeft = Quaternion.Euler(Vector3.up * 270) * Quaternion.Euler(Vector3.forward * 180);



    static public void Init()
    {
        GameObject mainControler = GameObject.Find("MainControler");

        envControlerScr = (EnvironmentControler)mainControler.GetComponent(typeof(EnvironmentControler));
        GUIControlerScr = (GUIControler)mainControler.GetComponent(typeof(GUIControler));
        
        mainCameraScr = (MainCameraScript)GameObject.Find("MainCamera").GetComponent(typeof(MainCameraScript));
    }



    static public Scene GetCurrentScene()
    {
        return (SceneManager.GetActiveScene());
    }

    static public void SwitchScene(Scene scene)
    {
        SceneManager.LoadScene(scene.name);
    }


    static public void ShowMessage(string msg)
    {
        #if UNITY_EDITOR
            if (Application.isEditor)
                UnityEditor.EditorUtility.DisplayDialog("", msg, "Ok");
        #endif
    }
    static public void ShowMessage(string msg, int value)
    {
        ShowMessage(msg+"\nValue: "+value);
    }
    static public void ShowMessage(string msg, float value)
    {
        ShowMessage(msg + "\nValue: " + value);
    }
    static public void ShowMessage(string msg, double value)
    {
        ShowMessage(msg + "\nValue: " + value);
    }
}
