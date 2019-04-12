# Fractional Clock Multiplier (wip)
8hp algorithmic rhythm-generation module for Eurorack, powered by the Teensy LC

Currently two main modes:
* Fractional Multiplication and division of steady or slowly-varying clock inputs.
  * No requirement for clock input to be multiple ppqn, as the clock input is predictive. I'd like to update this so it can deal with swung input also
* Generation of euclidean rhythms ( see [Toussaint 2005](http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf) ) from arbitrary (regular or non-regular) clock input 

Output can be shifted either by beat-quantized or subquantized amounts, and optionally recombined with the input. This can be used to generate swung clock output for example, or controlled via CV to generate complex compound rhythms. 

Selectable trigger/gate output modes. 

Assignable CV control over all paramaters, with attenuverter. 

Current schematic, panel mockup and PCB design:

<img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/Schematic.png" width="800" />

<img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Panel%20mockup.png" width="150" /> <img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/topPCB.png" width="150" /> <img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/bottomPCB.png" width="150" /> 

<img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/render.png" width="400" />


An older prototype:

<img src="https://lh3.googleusercontent.com/F2FJkoGD_2Y6mUCA_72Tr9jKIQL8T51u-yPpXMLdA85AF7AUzsiI0R6vGA8PAkv26-s6BQ4YR2C6YWq7D_WIugwAtMuLFM5RAIA3TbphAoilKe7S2X2kTRAt2BslmQAuLZtmlxdI60x7pljAAfFQjpe4ux4xAhEyvlYldTjiMu7CKEfx0s5RLvJsloXXFzuWYh74JVn_ynYBdR5kknDmDOABy-btkMf6ygelY21k-5Sk13OeYi_4nqbalsbw_Kay20YbHjPqkHRQ4EJwRg-HghsKRhGDjyb66y0XgOmLnHMniq8GFPWXzc7Ou17aOJmmEo-iptwhYquIdqMTw5126-4uQ4JN-fbZk5uMNa9TVr03RODQvABWOTEXRQ_gDacrYwKTu132whdYeGTQyy62glm7S0VYDMBLstyA9ZoxIxASJSf1TfdZvlR_91Kjug58e2I4FfsrTAtsJv_XrMFEXS_7mEBVeCH8DtkX2JoZfap947LTTdUUM3774dLUCF0JQwhfHeQYcQB1SFDA_VuOWtunsMsptIcdduzY5Tzj0rUNz_ZlazPewN_-AXO6eQbNcRoDB-4Ui7olnvTiXXzi2Du11re9mQ5SKkCuvxOnhPKEgmTtcn4kvNJcX3Qh0wEJHBKgqk0u13XQsy_8YfT3gf7GZKwuK8Y=w1330-h998-no" width="400" /> 
<img src="https://lh3.googleusercontent.com/jPlmeRyYHMEHQya63Dx3hL95Jbysh51vPWjdv0qWgl-WZDnvFw_E7UjdrqJFGYYVarH7iVs4ytmAIt0LABBTMHH6kHqQVGqYLSZLEW5gPa8H3oHkbPxZTMx4I1T8MirXX7wVHkFoZWimfaWsqIIgXPTuIJBbMEjeAzE6sdcyDQqNuGQdyXbF5uvd4dFsKb_3YRW6z8pEORVFEjMzekyBIoWQ0fae7UkBQnq0ZUeUsCjOyUTfJT4XScNNX3XcO8POUDrKwBvbkCi99yS22Z2fRHsxLioD7h6N1Tmqi0g-oB0sq_DcU2eDUo2e5Gb--74-7LbpuXSUz5ogKjD6iEGw0YnRVUEzRduWnBp71nQx6NfPfPymkGFj7QnaaJ3NyCzc8BOMhg7e9cG51Zjv-tQhav2DePmTunMwQUr27Ihcj0yp49WF03-9ZvHfPJ3tDSSp0pwqf31c9F4NayC2lvPpkkiMVsOQTXKi6ljk3kYxlaiO2Fg_8TIDArlpTC2xMDEHu4DV2MgCgSRlWVBEi4ztONQhaF4s8LsXI4rtHVcNCxsFCfL-9D9fdosnfCruD826B8dtw0TuZPW_jOtZ0NKaQcxt5UMTQD7lKvYxRfEgLSBl8_rl6_8IortW-olTqhssQ1F4xKqM6MwXBvdBJUlQVd7ExVZlULE=w1330-h998-no" width="400" />
