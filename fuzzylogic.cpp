#include <algorithm>
#include "fuzzylogic.h"

/////////////////////////////////////////////////////////////////
//
// okay so this is the fuzzy controller. it decides how hard to push
// the cart based on two combined inputs (Yamakawa style).
//
// X = A*theta + B*theta_dot -> this is basically "how bad is the pole falling"
// Y = C*x + D*x_dot -> this is basically "how bad is the cart drifting away"
//
// each input gets split into 5 vibes: NL NS ZE PS PL
// (very negative / a bit negative / basically zero / a bit positive / very positive)
//
// the output (force) has 9 possible levels from super negative to super positive.
// we make 25 rules total because 5 x 5 = every combo of X vibe and Y vibe.
//
/////////////////////////////////////////////////////////////////

// just a lookup list so we dont have to type in_nl in_ns etc 25 times below
static const int FUZZY_SET_ORDER[5] = {in_nl, in_ns, in_ze, in_ps, in_pl};

// giving each vibe a number so we can do math with it later
// NL = -2 (bad in the negative direction) all the way up to PL = 2
static const int SET_LEVEL[5]       = {-2,    -1,     0,     1,    2};

// this is just the 9 output levels in order from most negative to most positive
// this order actually matches the enum in the header file so index 0 to 8
// literally equals sum -4 to +4, kinda convenient not gonna lie
static const int OUTPUT_SET_ORDER[9] = {
    out_nvl, out_nl, out_nm, out_ns, out_ze, out_ps, out_pm, out_pl, out_pvl
};

// this function builds all 25 rules automatically so we dont have to
// write them out by hand one by one lol
void initFuzzyRules(fuzzy_system_rec *fl) {

    int ruleIndex = 0;

    // going through every X vibe and every Y vibe
    // this covers all 25 combos possible (5 times 5)
    for (int xi = 0; xi < 5; xi++) {
        for (int yi = 0; yi < 5; yi++) {

            // rule input number 1 is always X
            fl->rules[ruleIndex].inp_index[0]     = INPUT_X;
            fl->rules[ruleIndex].inp_fuzzy_set[0] = FUZZY_SET_ORDER[xi];

            // rule input number 2 is always Y
            fl->rules[ruleIndex].inp_index[1]     = INPUT_Y;
            fl->rules[ruleIndex].inp_fuzzy_set[1] = FUZZY_SET_ORDER[yi];

            // this is the actual "brain" of the rule.
            // we care about the pole (X) twice as much as the cart (Y)
            // since a fallen pole = game over but a drifting cart is not as urgent
            int sum = (2 * SET_LEVEL[xi]) + SET_LEVEL[yi]; // can go from -6 to +6

            // we only have output levels from -4 to +4 so we cap it here
            if (sum < -4) sum = -4;
            if (sum >  4) sum =  4;

            fl->rules[ruleIndex].out_fuzzy_set = OUTPUT_SET_ORDER[sum + 4];

            ruleIndex++;
        }
    }

    // at this point ruleIndex should be 25, matching fl->no_of_rules
    return;
}


// this sets up the actual shapes for each fuzzy set (the membership functions)
// think of these as "how much does this number count as NL or ZE or whatever"
void initMembershipFunctions(fuzzy_system_rec *fl) {

    // both X and Y use the exact same shape setup, spread across -4 to 4
    // picture 5 triangle shapes lined up next to each other like this:
    //
    //   NL   NS    ZE    PS   PL
    // 1 |\   /\    /\    /\   /|
    //   | \ /  \  /  \  /  \ / |
    // 0 |__X____X____X____X___|
    //  -4  -2    0    2    4
    //
    // NL: full strength (1.0) at -4 and below then fades to 0 by -2
    // NS: triangle peaking at -2
    // ZE: triangle peaking at 0
    // PS: triangle peaking at 2
    // PL: fades in from 2 and hits full strength (1.0) at 4 and above

    // ---- setting up input X (the combined pole angle stuff) ----
    fl->inp_mem_fns[INPUT_X][in_nl] = init_trapz(-4, -2, 0, 0, left_trapezoid);
    fl->inp_mem_fns[INPUT_X][in_ns] = init_trapz(-4, -2, -2, 0, regular_trapezoid);
    fl->inp_mem_fns[INPUT_X][in_ze] = init_trapz(-2, 0, 0, 2, regular_trapezoid);
    fl->inp_mem_fns[INPUT_X][in_ps] = init_trapz(0, 2, 2, 4, regular_trapezoid);
    fl->inp_mem_fns[INPUT_X][in_pl] = init_trapz(2, 4, 0, 0, right_trapezoid);

    // ---- setting up input Y (the combined cart position stuff) ----
    // literally the same shapes as X, just applied to a different input
    fl->inp_mem_fns[INPUT_Y][in_nl] = init_trapz(-4, -2, 0, 0, left_trapezoid);
    fl->inp_mem_fns[INPUT_Y][in_ns] = init_trapz(-4, -2, -2, 0, regular_trapezoid);
    fl->inp_mem_fns[INPUT_Y][in_ze] = init_trapz(-2, 0, 0, 2, regular_trapezoid);
    fl->inp_mem_fns[INPUT_Y][in_ps] = init_trapz(0, 2, 2, 4, regular_trapezoid);
    fl->inp_mem_fns[INPUT_Y][in_pl] = init_trapz(2, 4, 0, 0, right_trapezoid);

    return;
}

void initFuzzySystem (fuzzy_system_rec *fl) {

   // basic setup numbers for how big our system is
   fl->no_of_inputs = 2;  // we only ever look at 2 inputs at once (X and Y)
   fl->no_of_rules = 25;  // 5 X vibes times 5 Y vibes
   fl->no_of_inp_regions = 5;  // NL NS ZE PS PL
   fl->no_of_outputs = 9;  // 9 possible force levels

   // these coefficients turn the raw physics numbers into our combined X and Y
   //   X = A*theta + B*theta_dot
   //   Y = C*x + D*x_dot
   // heads up these are just a starting guess. you WILL need to tweak these
   // during calibration so the real values actually land inside -4 to 4
   coefficient_A = 3.0;   // how much pole angle matters
   coefficient_B = 1.0;   // how much pole angular speed matters
   coefficient_C = 1.0;   // how much cart position matters
   coefficient_D = 1.0;   // how much cart speed matters

   // how strong the push is for each output level, in Newtons
   // basically NVL pushes super hard left and PVL pushes super hard right
   fl->output_values[out_nvl] = -100.0;
   fl->output_values[out_nl]  = -75.0;
   fl->output_values[out_nm]  = -50.0;
   fl->output_values[out_ns]  = -25.0;
   fl->output_values[out_ze]  =   0.0;
   fl->output_values[out_ps]  =  25.0;
   fl->output_values[out_pm]  =  50.0;
   fl->output_values[out_pl]  =  75.0;
   fl->output_values[out_pvl] = 100.0;

   fl->rules = (rule *) malloc ((size_t)(fl->no_of_rules*sizeof(rule)));

   // this line was missing in the original start up code btw
   // without it free_fuzzy_rules never actually frees the memory
   fl->allocated = true;

   initFuzzyRules(fl);
   initMembershipFunctions(fl);
   return;
}

//////////////////////////////////////////////////////////////////////////////
// everything below here is the actual engine that came with the start up code
// you dont need to touch any of this. its just doing the math for us
//////////////////////////////////////////////////////////////////////////////

trapezoid init_trapz (float x1,float x2,float x3,float x4, trapz_type typ) {
	
   trapezoid trz;
   trz.a = x1;
   trz.b = x2;
   trz.c = x3;
   trz.d = x4;
   trz.tp = typ;
   switch (trz.tp) {
	   
      case regular_trapezoid:
         	 trz.l_slope = 1.0/(trz.b - trz.a);
         	 trz.r_slope = 1.0/(trz.c - trz.d);
         	 break;
	 
      case left_trapezoid:
         	 trz.r_slope = 1.0/(trz.a - trz.b);
         	 trz.l_slope = 0.0;
         	 break;
	 
      case right_trapezoid:
         	 trz.l_slope = 1.0/(trz.b - trz.a);
         	 trz.r_slope = 0.0;
         	 break;
   }  /* end switch  */
   
   return trz;
}  /* end function */

//////////////////////////////////////////////////////////////////////////////
// works out how strongly a number "belongs" to a fuzzy shape
// gives back something between 0 (not at all) and 1 (totally)
//////////////////////////////////////////////////////////////////////////////
float trapz (float x, trapezoid trz) {
   switch (trz.tp) {
	   
      case left_trapezoid:
         	 if (x <= trz.a)
         	    return 1.0;
         	 if (x >= trz.b)
         	    return 0.0;
         	 /* a < x < b */
         	 return trz.r_slope * (x - trz.b);
	 
	 
      case right_trapezoid:
         	 if (x <= trz.a)
         	    return 0.0;
         	 if (x >= trz.b)
         	    return 1.0;
         	 /* a < x < b */
         	 return trz.l_slope * (x - trz.a);
	 
      case regular_trapezoid:
         	 if ((x <= trz.a) || (x >= trz.d))
         	    return 0.0;
         	 if ((x >= trz.b) && (x <= trz.c))
         	    return 1.0;
         	 if ((x >= trz.a) && (x <= trz.b))
         	    return trz.l_slope * (x - trz.a);
         	 if ((x >= trz.c) && (x <= trz.d))
         	    return  trz.r_slope * (x - trz.d);
         	    
	 }  /* End switch  */
	 
   return 0.0;  /* should not get to this point */
}  /* End function */

//////////////////////////////////////////////////////////////////////////////
// just grabs the smallest value out of a list
// used because fuzzy AND means "take the weakest link"
//////////////////////////////////////////////////////////////////////////////
float min_of(float values[],int no_of_inps) {
   int i;
   float val;
   val = values [0];
   for (i = 1;i < no_of_inps;i++) {
       if (values[i] < val)
	  val = values [i];
   }
   return val;
}



//////////////////////////////////////////////////////////////////////////////
// this is the main function that runs the whole fuzzy system
// it checks every rule then blends the results into one final force number
//////////////////////////////////////////////////////////////////////////////
float fuzzy_system (float inputs[],fuzzy_system_rec fz) {
   int i,j;
   short variable_index,fuzzy_set;
   float sum1 = 0.0,sum2 = 0.0,weight;
   float m_values[MAX_NO_OF_INPUTS];
	
   // go through every single rule one at a time
   for (i = 0;i < fz.no_of_rules;i++) {
      for (j = 0;j < fz.no_of_inputs;j++) {
	   variable_index = fz.rules[i].inp_index[j];
	   fuzzy_set = fz.rules[i].inp_fuzzy_set[j];
	   // check how much this rule's input actually matches right now
	   m_values[j] = trapz(inputs[variable_index],
	       fz.inp_mem_fns[variable_index][fuzzy_set]);
	   } /* end j  */
      
       // fuzzy AND just means take the weakest match out of the inputs
       weight = min_of (m_values,fz.no_of_inputs);
				
       // add this rule's vote to the running total, weighted by how much it matched
       sum1 += weight * fz.output_values[fz.rules[i].out_fuzzy_set];
       sum2 += weight;
   } /* end i  */
 
	// safety check so we dont divide by basically zero
	if (fabs(sum2) < TOO_SMALL) {
      cout << "\r\nFLPRCS Error: Sum2 in fuzzy_system is 0.  Press key: " << endl;
      //~ getch();
      //~ exit(1);
      return 0.0;
   }
   
   // this is the weighted average aka the defuzzification step
   return (sum1/sum2);
}  /* end fuzzy_system  */

//////////////////////////////////////////////////////////////////////////////
// cleans up the memory we grabbed for the rules array
//////////////////////////////////////////////////////////////////////////////
void free_fuzzy_rules (fuzzy_system_rec *fz) {
   if (fz->allocated){
	   free (fz->rules);
	}
	
   fz->allocated = false;
   return;
}
