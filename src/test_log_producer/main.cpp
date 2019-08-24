#include <iostream>
#include <fstream>
#include <cstdio>
#include <libgen.h>

#include "Log_service.h"
#include "wrap.h"
#include "Config.h"

using namespace std;

char ID[KEY_LEN];
int shm_num = 0;

void test();
int main( int argc, char* argv[] )
{
    
    if( argc <= 2 )
    {
        printf( "usage: %s ID shm_num\n", basename( argv[0] ) );
        return 1;
    }
    lgy_strncpy(ID, argv[1], KEY_LEN);
    shm_num = atoi( argv[2] );

    init_cody_log_service(ID, shm_num);
    // ifstream read_file;
    // read_file.open("./test_log.txt", ios::binary);


    for(int i=0; i < 1000; i++){
        std::cout<< ID << ' ' << i <<std::endl;
        for(int j=0; j<1000; j++)
        {        
            test();
        }
    }

    // read_file.close();

    // for(int i=0; i<500; i++){

    //     std::cout<< i <<std::endl;

    //     for(int j=0; j<100000; j++){
    //         cody_log("%d\n", i*350+j);
    //     }
    // }

    printf("%s done\n", ID);

    // debug_send_persistense();
    close_cody_log_service();
}


void test()
{
    cody_log("%s", "He has refused his Assent to Laws, the most wholesome and necessary for the public good.");
    cody_log("%s", "He has forbidden his Governors to pass Laws of immediate and pressing importance, unl");
    cody_log("%s", "He has refused to pass other Laws for the accommodation of large districts of people,");
    cody_log("%s", "He has called together legislative bodies at places unusual, uncomfortable, and dista");
    cody_log("%s", "He has dissolved Representative Houses repeatedly, for opposing with manly firmness o");
    cody_log("%s", "He has refused for a long time, after such dissolutions, to cause others to be electe");
    cody_log("%s", "He has endeavoured to prevent the population of these States; for that purpose obstru");
    cody_log("%s", "He has obstructed the Administration of Justice by refusing his Assent to Laws for es");
    cody_log("%s", "He has made Judges dependent on his Will alone for the tenure of their offices, and t");
    cody_log("%s", "He has erected a multitude of New Offices, and sent hither swarms of Officers to hara");
    cody_log("%s", "He has kept among us, in times of peace, Standing Armies without the Consent of our l");
    cody_log("%s", "He has affected to render the Military independent of and superior to the Civil Power");
    cody_log("%s", "He has combined with others to subject us to a jurisdiction foreign to our constituti");
    cody_log("%s", "For quartering large bodies of armed troops among us:");
    cody_log("%s", "For protecting them, by a mock Trial from punishment for any Murders which they shoul");
    cody_log("%s", "For cutting off our Trade with all parts of the world:");
    cody_log("%s", "For imposing Taxes on us without our Consent:");
    cody_log("%s", "For depriving us in many cases, of the benefit of Trial by Jury:");
    cody_log("%s", "For transporting us beyond Seas to be tried for pretended offences:");
    cody_log("%s", "For abolishing the free System of English Laws in a neighbouring Province, establishi");
    cody_log("%s", "For taking away our Charters, abolishing our most valuable Laws and altering fundamen");
    cody_log("%s", "For suspending our own Legislatures, and declaring themselves invested with power to ");
    cody_log("%s", "He has abdicated Government here, by declaring us out of his Protection and waging Wa");
    cody_log("%s", "He has plundered our seas, ravaged our coasts, burnt our towns, and destroyed the liv");
    cody_log("%s", "He is at this time transporting large Armies of foreign Mercenaries to compleat the w");
    cody_log("%s", "He has constrained our fellow Citizens taken Captive on the high Seas to bear Arms ag");
    cody_log("%s", "Life in the world is but a big dream;");
    cody_log("%s", "I will not spoil it by any labour or care.");
    cody_log("%s", "So saying, I was drunk all the day,");
    cody_log("%s", "lying helpless at the porch in front of my door.");
    cody_log("%s", "When I awoke, I blinked at the garden-lawn;");
    cody_log("%s", "a lonely bird was singing amid the flowers.");
    cody_log("%s", "I asked myself, had the day been wet or fine?");
    cody_log("%s", "The Spring wind was telling the mango-bird.");
    cody_log("%s", "Moved by its song I soon began to sigh,");
    cody_log("%s", "and, as wine was there, I filled my own cup.");
    cody_log("%s", "Wildly singing I waited for the moon to rise;");
    cody_log("%s", "when my song was over, all my senses had gone.");
    cody_log("%s", "Bright shines the Moon before my bed;");
    cody_log("%s", "Methinks ’tis frost upon the earth.");
    cody_log("%s", "I watch the Moon, then bend my head");
    cody_log("%s", "And miss the hamlet of my birth.");
    cody_log("%s", "A cup of wine, under the flowering trees;");
    cody_log("%s", "I drink alone, for no friend is near.");
    cody_log("%s", "Raising my cup I beckon the bright moon,");
    cody_log("%s", "For he, with my shadow, will make three men.");
    cody_log("%s", "The moon, alas, is no drinker of wine;");
    cody_log("%s", "Listless, my shadow creeps about at my side.");
    cody_log("%s", "Yet with the moon as friend and the shadow as slave");
    cody_log("%s", "I must make merry before the Spring is spent.");
}