using UnityEngine;
using System.Collections;
using System.Collections.Generic;


/*
 * The GlobalSettings holds a lot of globally available, static variables.
 */


public class GlobalSettings : MonoBehaviour {

    // Point of origin for the whole scene
    static public Vector3 originPoint { get; private set; }

    static public Color focusColor = new Color(0, 1, 0, 1);

    static public string language = "en";

    static public void Init()
    {

        GameObject basePoint = GameObject.Find("Basepoint");
        if (basePoint != null)
            originPoint = basePoint.transform.position;
        else originPoint = Vector3.zero;


        // Initializing physik values
        Time.timeScale = 1.5f;
        //Time.fixedDeltaTime = standardFixedPhysicDeltaTime;

    }
}
