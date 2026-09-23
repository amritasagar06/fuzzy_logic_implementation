1. What This Project Does (the big picture)

Imagine balancing a broomstick on your palm. You watch it tip, and you move your hand a little to keep it upright. That's exactly what this program does, except:

The "hand" is a cart that can move left and right on a track
The "broomstick" is a pole attached to the cart with a hinge
Instead of your brain deciding how to move your hand, a fuzzy logic controller decides how much force to push the cart with

The program simulates the physics of this cart-pole system (using real equations of motion, not made up), and your job was to write the "brain" that decides the push force at every instant, based on how tilted the pole is and how fast it's tipping.

2. How the Program is Organized
File	What it does
main.cpp	Runs the simulation loop, draws the graphics, reads your typed angle, calls the physics equations, and calls YOUR controller every frame
fuzzylogic.h	Defines the "shapes" (structs/enums) the fuzzy engine understands (rules, fuzzy sets, trapezoid shapes)
fuzzylogic.cpp	This is what you wrote. It defines the actual rules, membership functions, and coefficients
transform.cpp / sprites.cpp / graphics.cpp	Just drawing code (cart, pole, background). You never touch this
makefile	Tells the compiler how to build everything into one program
3. Step-by-Step: What Happens When You Run It
You type an angle, like 20 (meaning the pole starts tilted 20° from upright)
The program converts that to radians and sets the pole's starting position
Every "frame" (a tiny slice of time, like 1/50th of a second), the program:
Reads the pole's current angle and angular velocity, and the cart's position and velocity
Combines them into two numbers: X (about the pole) and Y (about the cart)
Feeds X and Y into your fuzzy controller
Your controller outputs one number: how much force to push the cart with
The physics equations use that force to calculate the new angle, position, etc for the next frame
This repeats until either:
The pole and cart settle down near zero → "Balanced!"
The pole falls past 90° or the cart drives off the track edges → failure
4. Key Concepts Explained Simply
What is "fuzzy logic"?

Normal code uses hard rules: if angle > 5, push right. Fuzzy logic uses soft, human-like rules: if the pole is "a little bit" tilted right, push "a little bit" right. Instead of a number being either "true" or "false" for a category, it can be like 70% "big tilt" and 30% "small tilt" at the same time. The system blends all these partial truths together to get one final answer.

What are X and Y?

Rather than using 4 separate raw values (angle, angular velocity, position, velocity), you combine them into just 2 numbers using simple weighted formulas:

X = A*theta + B*theta_dot → one number that summarizes "how urgently is the pole falling"
Y = C*x + D*x_dot → one number that summarizes "how urgently is the cart drifting away"

This is a trick from Yamakawa's original paper. It simplifies a 4-input problem into a 2-input problem.

What are "membership functions"?

These are just shapes (in our case, triangles and trapezoids) that convert a raw number like X = 1.5 into a "fuzzy label" like 70% Positive-Small, 30% Positive-Large. Think of it like a dimmer switch instead of an on/off switch.

What are the 5 fuzzy sets (NL, NS, ZE, PS, PL)?

They're just labels for "how big and in which direction":

NL = Negative Large
NS = Negative Small
ZE = Zero (basically none)
PS = Positive Small
PL = Positive Large
What is a "rule"?

A simple sentence like: "IF X is Negative Large AND Y is Zero, THEN push with Negative Large force." You have 25 of these (one for every combination of X's 5 labels and Y's 5 labels).

What does "Zero-order Sugeno" mean?

There are different types of fuzzy systems. In a Sugeno system, each rule's output is a fixed constant number (like -100 or +50), instead of another fuzzy shape. This makes the math simpler and faster, since you just need a weighted average at the end instead of a more complex "centroid of a shape" calculation.

What is "defuzzification"?

After all 25 rules "vote" with different strengths, you need ONE final number (the actual force to apply). Defuzzification is the averaging process that turns many fuzzy votes into one crisp number. This project uses weighted average: each rule's suggested output is multiplied by how strongly that rule applies, then everything is summed up and divided by the total "vote weight."

5. Viva Questions & Answers
Basic Concepts

Q: What is fuzzy logic and why use it here instead of normal control theory (like PID)? A: Fuzzy logic lets you control a system using human-like, descriptive rules ("if angle is a bit too far, push a bit") instead of precise mathematical formulas. It's useful when the system is nonlinear (like a pendulum, which behaves very differently near vertical vs. near horizontal) and when it's easier to describe good behavior in words than in exact equations.

Q: What's the difference between Mamdani and Sugeno fuzzy systems? A: In Mamdani systems, both the inputs AND outputs are fuzzy shapes, so you need to "defuzzify" a whole output shape at the end (usually with centroid method). In a Zero-order Sugeno system, the outputs are just fixed numbers (constants), so the final answer is simply a weighted average, which is computationally simpler and faster.

Q: Why does this project use combined inputs X and Y instead of the 4 raw values (theta, theta_dot, x, x_dot)? A: This follows Yamakawa's original design. Combining 4 variables into 2 combined ones (X for the pole, Y for the cart) drastically reduces the number of rules needed. With 4 separate inputs and 5 regions each, you'd need up to 5^4 = 625 rules. With 2 combined inputs, you only need 5×5 = 25 rules.

Q: What do A, B, C, D represent? A: They're scaling coefficients that decide how much weight the angle vs. angular velocity (and position vs. velocity) get when combined into X and Y. They're tuned so that typical real values (like angle in radians, which is usually small) map nicely into the fuzzy sets' range of -4 to +4.

About the Membership Functions

Q: What shape did you use for your membership functions, and why? A: Trapezoidal shapes (with triangles as a special case where the flat top is zero-width). They're simple to compute and were already provided by the engine's trapz() function. Triangles/trapezoids are also the most common choice in real fuzzy control systems because they're computationally cheap.

Q: How many fuzzy sets does each input have, and what are they? A: 5 each: NL (Negative Large), NS (Negative Small), ZE (Zero), PS (Positive Small), PL (Positive Large).

Q: What range are your inputs defined over? A: -4 to +4, as suggested in the assignment brief. Both X and Y use the same range and same shapes.

About the Rules

Q: How many rules do you have, and how did you generate them? A: 25 rules, covering every combination of the 5 regions for X and 5 regions for Y. They were generated systematically: each rule's output level is calculated as 2×level(X) + level(Y), clipped to a valid range. This means the pole angle (X) is weighted twice as strongly as the cart position (Y), since keeping the pole upright is more urgent than keeping the cart centered.

Q: Why weight X twice as much as Y? A: If the pole falls all the way over, the simulation fails immediately and can't be recovered. If the cart drifts a bit, there's usually still time to correct it. So reacting strongly to pole tilt is more important than reacting strongly to cart drift.

Q: What is the "AND" operation in your rules, and how is it computed? A: Each rule has two conditions (about X and about Y) joined by AND. In fuzzy logic, AND is typically computed as the minimum of the two membership values, since a rule's overall truth can't be stronger than its weakest condition. This is the min_of() function in the code.

About the Output and Defuzzification

Q: What defuzzification method did you use? A: Weighted average. Since this is a Sugeno system with constant outputs, this method is mathematically equivalent to centroid defuzzification.

Q: What are your output force values? A: 9 constant levels from -100N to +100N in steps of 25N, corresponding to NVL, NL, NM, NS, ZE, PS, PM, PL, PVL.

Q: What happens if no rule fires strongly (i.e., all weights are near zero)? A: The code has a safety check: if the total weight (sum2) is too small, it returns 0 force instead of dividing by nearly zero (which would cause a math error or huge/unstable output).

About Calibration and Results

Q: How did you test your controller's limits? A: By running the simulation with different starting angles and checking whether it balances or fails (pole falls past 90°, or the cart exceeds the track's [-2.4, 2.4] boundary).

Q: What was your final working range, and why is it asymmetric? A: My controller balanced angles from about -52° to +21°. It's asymmetric because the cart always starts at x=1.0, not at the center. Since the boundary is at ±2.4 (with a small safety margin), the cart has much more room to move left (about 3.4 units) than right (about 1.4 units), so it survives bigger negative tilts before running out of room.

Q: What would you change to improve the range further? A: Increase the sensitivity coefficients (like coefficient_A) so the controller reacts more strongly to larger tilts, or adjust the rule table to apply stronger force sooner for large-angle situations.

Q: What happens if the simulation seems to hang or freeze? A: This can happen if the pole oscillates forever without settling AND without exceeding the failure boundaries. It's not a crash, the loop is technically still running, just never triggers either "Balanced!" or a failure condition. Pressing Escape forces it to stop.

About the Code Structure

Q: Which parts of the code were you not allowed to modify, and why? A: The physics engine (the equations calculating angular acceleration, cart acceleration, etc. based on real cart-pole dynamics) had to stay untouched, since that represents the "real world" the controller has to work against. Only the fuzzy logic (rules, membership functions, coefficients) was meant to be written by the student.

Q: What's the purpose of initFuzzySystem()? A: It's the setup function that runs once at the start. It defines how many inputs/rules/outputs the system has, sets the A/B/C/D coefficients, sets the output force values, allocates memory for the rules array, and then calls initFuzzyRules() and initMembershipFunctions() to actually fill in the details.

Q: Why was fl->allocated = true important to add? A: Without it, the free_fuzzy_rules() cleanup function would never actually free the memory that was allocated for the rules array, since it checks that flag before freeing. It's a small memory-safety fix.

6. If They Ask "Walk Me Through Your Code Live"

A safe way to narrate it:

Open fuzzylogic.cpp, point to initFuzzySystem() first. Explain it sets up the basic numbers and coefficients.
Point to initMembershipFunctions(). Explain the 5 triangle/trapezoid shapes and why they're spread from -4 to 4.
Point to initFuzzyRules(). Explain the double loop building 25 rules and the 2×levelX + levelY logic.
Point to fuzzy_system() (the engine function). Explain the loop: for each rule, check how well the current X and Y match (trapz), take the minimum (AND), multiply by the rule's output value, sum everything up, and divide by the total weight at the end (weighted average / defuzzification).
Mention where this connects into main.cpp, i.e. it's called every simulation frame, and its return value becomes the force pushing the cart.

That's the whole story, from raw sensor numbers to a single force number, 25 rules at a time.
