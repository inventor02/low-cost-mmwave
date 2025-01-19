BigSep = 8e-3;
UpSep = sqrt((2*BigSep)^2-BigSep^2);
PL = 1049e-6;
PW = 1412e-6;
sub = dielectric(Name="RO3003", EpsilonR=3, LossTangent=0.0013, Thickness=130e-6);
con = metal(Name="Copper", Conductivity=5.96e7, Thickness=17e-6);
WL = 4e-3;
OffVert = WL/4;
OffHori =WL/4;
Centerx = 0;
Centery = -0.5e-3;
%-------------

VertiSmall =1.5; %both val's betweem 1-2 
HoriSmall =1.4; 
%BEST COMBO IS 1.5 1.4
%--------------
BoardL = 4e-3*HoriSmall; % how WIDE
BoardW = 1.5e-3+4e-3*VertiSmall; %how TALL



RR.BoardShape = antenna.Rectangle(Center=[Centerx Centery],Length=BoardL,Width=BoardW);
RR.Layers{3} = RR.BoardShape;
ground = RR.BoardShape;


top = RR.Layers{1};
cut1 = antenna.Rectangle(Center=[34.4e-3 0e-3],Length=3e-3,Width=5e-3);
cut2 = antenna.Rectangle(Center=[-34.4e-3 0e-3],Length=3e-3,Width=5e-3);
top = top - cut1 - cut2;
top = rotateZ(top,-30);
cutfifine1 = antenna.Rectangle(Center=[0 35.9e-3],Length=7e-3,Width=6e-3);
top = top - cutfifine1;
top = rotateZ(top,-30);
cutfifine2 = antenna.Rectangle(Center=[35.9e-3 0],Length=6e-3,Width=7e-3);
top = top - cutfifine2;
top = rotateZ(top,30);
top = translate(top,[0,UpSep/2,0]);



ant = patchMicrostripInsetfed; %based off of designed values 
ant.Length = 0.9825079993e-3;
ant.Width = 1.274420879e-3;
ant.Height = 1.016e-4; %above 3 are correct
ant.Substrate = sub; %unsure here
ant.NotchLength = 2e-4;%Unsure how to calculate notch length/width, just reduced it greatly for now
ant.GroundPlaneLength = 2e-3;
ant.GroundPlaneWidth = 2e-3;
ant.StripLineWidth = 1e-4; %feedline diameter (0.1mm) 
ant.NotchWidth = 3.577e-4; %unsure how to calculate notch length/width
%show(ant)

%This is creating the array needed position wise 
BigSep = 8e-3;
UpSep = sqrt((2*BigSep)^2-BigSep^2);
array = pcbStack(ant);
a1 = array.Layers{1};
a1 = rotateZ(a1,90);
a2 = copy(a1);
a3 = copy(a1);
a4 = copy(a1);%copy first, and then translate (to a new name) to move 
a5 = copy(a1);
a6 = copy(a1);
%across, up, zero z ) 
ant1 = translate(a1,[OffHori*(HoriSmall) OffVert*(VertiSmall),0]); 
ant2 = translate(a2,[OffHori*(HoriSmall) -OffVert*(VertiSmall),0]); %these first two for the middle top ones that are wanted to be mirrored
ant3 = translate(a3,[BigSep-2e-3 2e-3, 0]);
ant4 = translate(a4,[BigSep+2e-3 2e-3, 0]);
ant5 = translate(a5,[BigSep-2e-3 -2e-3, 0]);
ant6 = translate(a6,[BigSep+2e-3 -2e-3, 0]);
antArrayleft = ant1 + ant2; % ant3 + ant4 + ant5 + ant6
antArrayright = copy(antArrayleft); %mirrored along y plane 
antArrayright = mirrorY(antArrayright);
antArray = antArrayleft + antArrayright;
figure(Name="Microstrip Patch Array")
show(antArray)
%done correctly it looks
%


%This is making the trace T junction shapes, and other connection shapes
%length is along, width is up / down
inline1 = antenna.Rectangle(Center=[0 -1.5e-3*VertiSmall],Length=2e-4,Width=3e-3);
inline2= antenna.Rectangle(Center=[-9.55e-3 1e-3],Length=1e-3,Width=1e-4);%upper bot left
inline3 = antenna.Rectangle(Center=[-1.55e-3 -WL/2],Length=1e-3,Width=1e-4);
inline4 = antenna.Rectangle(Center=[-9.55e-3 -3e-3],Length=1e-3,Width=1e-4); %bot left
inline5 = antenna.Rectangle(Center=[-6.45e-3 1e-3],Length=1e-3,Width=1e-4); %bot left
inline6 = antenna.Rectangle(Center=[-6.45e-3 -3e-3],Length=1e-3,Width=1e-4); %bot left
tee = traceTee;
tee.Length = [2e-3*HoriSmall+1e-4 1e-3*VertiSmall]; %length of the two T parts along and down 
tee.Width = [5e-5 2e-4];
tee.ReferencePoint = [0 -(1-VertiSmall)*1e-3];
tee1 = copy(tee);
tee1.ReferencePoint = [BigSep 1e-3];
tee2=traceTee;
tee2.Length = [2e-3*HoriSmall+1e-4 1e-3*VertiSmall]; %length of the two T parts along and down 
tee2.Width = [5e-5 2e-4];
tee2.ReferencePoint = [BigSep 3e-3]; 
tee2 = rotateZ(tee2,180); 
teeSide = traceTee; 
teeSide.Length = [2.5e-3 4e-3];
teeSide.Width = [1e-4 2e-4];
teeSide.ReferencePoint = [-1e-3 8e-3];
teeSide= rotateZ(teeSide,90);
teeTop = copy(teeSide); 
teeTop.ReferencePoint = [12.75e-3 0];
tee3 = copy(tee2);
tee3.ReferencePoint = [0 WL/2-(1-VertiSmall)*1e-3];

feedLeft = inline1 + tee  +tee3  ; %+ inline2 + inline3 + inline4 + inline5 + inline6 + tee1 +tee2 + teeSide inline1 + inline3 +
feedRight = copy(feedLeft);
feedRight = mirrorY(feedRight);
totaltop =feedLeft + feedRight + antArray; % top + 
R1=copy(totaltop);
R2=copy(totaltop);
R3 = copy(totaltop);
T2 =copy(totaltop);
T3 = copy(totaltop);
R4=copy(totaltop);  
translate(R2,[4.8e-3 2e-3 0]);%set vertical to 2 for patch - patch, and to 5 for upper right / lower left
translate(R3,[9.6e-3 -2e-3 0]); 
translate(R4, [14.4e-3 0 0]);

translate(T2,[6e-3 4e-3 0]); %9.6
translate(T3, [12e-3 0 0]); %19.2

totaltotaltop = R1+T2+T3; % R1+R2+R3+R4;
show(totaltotaltop) %run just this first section to see the antenna layout for either the transmit or recieve antenna's
%%
PatchOffset_mm = [2*OffHori*(HoriSmall)*1e3 2*OffVert*(VertiSmall)*1e3]
% final assembly / feed locations 
rrMicrostrip = pcbStack;
rrMicrostrip.BoardThickness = 130e-6;
rrMicrostrip.BoardShape = antenna.Rectangle(Center=[Centerx Centery],Length=BoardL,Width=BoardW); % Center=[0 7e-3],Length=25e-3,Width=22e-3
rrMicrostrip.Layers = {totaltop sub ground};
feedloc =  [0 -(1.5*VertiSmall+1.45)*1e-3 3 1]; %[-4e-3 -1e-3 3 1]; [4e-3 -1e-3 3 1];
rrMicrostrip.FeedLocations = feedloc;
rrMicrostrip.FeedDiameter = 1e-5;
rrMicrostrip.FeedViaModel = "strip";
rrMicrostrip.FeedPhase = 0;
%show(rrMicrostrip)
%openExample('rfpcb/DesignSBandMonopulseTrackingRADARAntennaPCBExample')
%
plotfrequency = 77*1e9; %77Ghz frequency bb
lambda = 3e8/plotfrequency;
mesh(rrMicrostrip,MaxEdgeLength=500e-6,GrowthRate=0.7);
memEst = memoryEstimate(rrMicrostrip,3e9) %run this section to make sure mesh size doesn't blow up computer (shouldn't above 140e-6) 
%%
%frequencyRange = (76:1:81)*1e9; 
%calculateSparameters = true;
%if calculateSparameters
%    s = sparameters(rrMicrostrip,frequencyRange);
%    rfplot(s)
%end


%Directivity = pattern(rrMicrostrip,77e9,0,90)
%[bw, angles] = beamwidth(rrMicrostrip,77e9,0,1:1:360)
figure
pattern(rrMicrostrip,plotfrequency)

%figure
%pattern(rrMicrostrip,77e9,0:1:360,0:1:10);
