/*
 * vmnetfs - virtual machine network execution virtual filesystem
 *
 * Copyright (C) 2006-2012 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * COPYING.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "vmnetfs-private.h"

#define IMAGE_ARG_COUNT 7

//#define DEBUG_STAND_ALONE 1

static void _image_free(struct vmnetfs_image *img)
{
    _vmnetfs_stream_group_free(img->io_stream);
    _vmnetfs_stat_free(img->bytes_read);
    _vmnetfs_stat_free(img->bytes_written);
    _vmnetfs_stat_free(img->chunk_fetches);
    _vmnetfs_stat_free(img->chunk_dirties);
    g_free(img->url);
    g_free(img->username);
    g_free(img->password);
    g_free(img->total_overlay_chunks);
    g_slice_free(struct vmnetfs_image, img);
}

static void image_free(struct vmnetfs_image *img)
{
    if (img == NULL) {
        return;
    }
    _vmnetfs_io_destroy(img);
    _image_free(img);
}

static uint64_t parse_uint(const char *str, GError **err)
{
    char *endptr;
    uint64_t ret;

    ret = g_ascii_strtoull(str, &endptr, 10);
    if (*str == 0 || *endptr != 0) {
        g_set_error(err, VMNETFS_CONFIG_ERROR,
                VMNETFS_CONFIG_ERROR_INVALID_ARGUMENT,
                "Invalid integer argument: %s", str);
        return 0;
    }
    return ret;
}

static char *read_line(GIOChannel *chan, GError **err)
{
    char *buf;
    gsize terminator;

    switch (g_io_channel_read_line(chan, &buf, NULL, &terminator, err)) {
    case G_IO_STATUS_ERROR:
        return NULL;
    case G_IO_STATUS_NORMAL:
        buf[terminator] = 0;
        return buf;
    case G_IO_STATUS_EOF:
        g_set_error(err, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_IO,
                "Unexpected EOF");
        return NULL;
    case G_IO_STATUS_AGAIN:
        g_set_error(err, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_IO,
                "Unexpected EAGAIN");
        return NULL;
    default:
        g_assert_not_reached();
    }
}

static char **get_arguments(GIOChannel *chan, GError **err)
{
    uint64_t n;
    uint64_t count;
    char *str;
    GPtrArray *args;
    GError *my_err = NULL;

    /* Get argument count */
    str = read_line(chan, err);
    if (str == NULL) {
        return NULL;
    }
    count = parse_uint(str, &my_err);
    if (my_err) {
        g_propagate_error(err, my_err);
        return NULL;
    }

    /* Get arguments */
    args = g_ptr_array_new_with_free_func(g_free);
    for (n = 0; n < count; n++) {
        str = read_line(chan, err);
        if (str == NULL) {
            g_ptr_array_free(args, TRUE);
            return NULL;
        }
        g_ptr_array_add(args, str);
    }
    g_ptr_array_add(args, NULL);
    return (char **) g_ptr_array_free(args, FALSE);
}

static struct vmnetfs_image *image_new(char **argv, const char *username,
        const char *password, GError **err)
{
    struct vmnetfs_image *img;
    int arg = 0;
    GError *my_err = NULL;

    const char *url = argv[arg++];
    const char *base_path = argv[arg++];
    const char *overlay_path = argv[arg++];
    const char *overlay_info = argv[arg++];
    const uint64_t size = parse_uint(argv[arg++], &my_err);
    if (my_err) {
        g_propagate_error(err, my_err);
        return NULL;
    }
    const uint64_t segment_size = parse_uint(argv[arg++], &my_err);
    if (my_err) {
        g_propagate_error(err, my_err);
        return NULL;
    }
    const uint32_t chunk_size = parse_uint(argv[arg++], &my_err);
    if (my_err) {
        g_propagate_error(err, my_err);
        return NULL;
    }

    const int base_fd = open(base_path, O_RDONLY);
    if (base_fd < 0){
        g_set_error(err, VMNETFS_CONFIG_ERROR,
                VMNETFS_CONFIG_ERROR_INVALID_ARGUMENT,
                "Invalid path for base image: %s", base_path);
        return NULL;
    }
    int overlay_fd = -1;
    if (strlen(overlay_path) != 0){
        overlay_fd = open(overlay_path, O_RDONLY);
        if (overlay_fd < 0){
        	g_set_error(err, VMNETFS_CONFIG_ERROR,
        			VMNETFS_CONFIG_ERROR_INVALID_ARGUMENT,
        			"Invalid path for overlay image: %s", overlay_path);
        	return NULL;
        }
    }

    img = g_slice_new0(struct vmnetfs_image);
    img->url = g_strdup(url);
    img->username = g_strdup(username);
    img->password = g_strdup(password);
    img->total_overlay_chunks = g_strdup(overlay_info);
    img->initial_size = size;
    img->segment_size = segment_size;
    img->chunk_size = chunk_size;
    img->base_fd = base_fd;
    img->overlay_fd = overlay_fd;

    img->io_stream = _vmnetfs_stream_group_new(NULL, NULL);
    img->bytes_read = _vmnetfs_stat_new();
    img->bytes_written = _vmnetfs_stat_new();
    img->chunk_fetches = _vmnetfs_stat_new();
    img->chunk_dirties = _vmnetfs_stat_new();

    if (!_vmnetfs_io_init(img, err)) {
        _image_free(img);
        return NULL;
    }

    return img;
}

static void image_close(struct vmnetfs_image *img)
{
    _vmnetfs_io_close(img);
    _vmnetfs_stat_close(img->bytes_read);
    _vmnetfs_stat_close(img->bytes_written);
    _vmnetfs_stat_close(img->chunk_fetches);
    _vmnetfs_stat_close(img->chunk_dirties);
    _vmnetfs_stream_group_close(img->io_stream);
}

static void *glib_loop_thread(void *data)
{
    struct vmnetfs *fs = data;

    fs->glib_loop = g_main_loop_new(NULL, TRUE);
    g_main_loop_run(fs->glib_loop);
    g_main_loop_unref(fs->glib_loop);
    fs->glib_loop = NULL;
    return NULL;
}

static void handle_stdin(struct vmnetfs *fs, const char *oneline, GError **err){
	// valid format : (image_index: chunk_number),
	// ex) 1:10251
    struct vmnetfs_image *img;
	u_int image_index = -1;
	guint64 chunk_number = -1;

	gchar *end;
	gchar **overlay_info = g_strsplit(oneline, ":", 0);
	// 1 for disk, 2 for memory
	image_index = (int) g_ascii_strtoull(*(overlay_info), &end, 10);
	if (*overlay_info == end) {
		g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
				"Invalid overlay format at image index %s", *(overlay_info + 1));
	}

	chunk_number = g_ascii_strtoull(*(overlay_info+1), &end, 10);
	if (*overlay_info == end) {
		g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
				"Invalid overlay format at chunk number %s", *overlay_info);
	}


	if(image_index == 1){
		img = fs->disk;
	}else if(image_index == 2){
		img = fs->memory;
	}else{
		g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
				"Invalid index number %d", image_index);
		return;
	}

	// Set bit for total_overlay_map
    _vmnetfs_bit_set(img->current_overlay_map, chunk_number);
    printf("update overlay at chunk(%d : %lu)\n", image_index,  chunk_number);

}

static gboolean read_stdin(GIOChannel *source,
        GIOCondition cond G_GNUC_UNUSED, void *data)
{
    struct vmnetfs *fs = data;
    char *buf;
    gsize terminator;
    GError *err = NULL;

    /* See if stdin has been closed. */
    do {
    	switch (g_io_channel_read_line(source, &buf, NULL, &terminator, &err)) {
    	    case G_IO_STATUS_ERROR:
    	        return TRUE;
    	    case G_IO_STATUS_NORMAL:
    	        buf[terminator] = 0;
    	        break;
    	    case G_IO_STATUS_EOF:
    	        g_set_error(err, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_IO,
    	                "Unexpected EOF");
    	        terminator = 0;
    	        break;
    	    case G_IO_STATUS_AGAIN:
    	        g_set_error(err, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_IO,
    	                "Unexpected EAGAIN");
    	        return TRUE;
    	    default:
    	        g_assert_not_reached();
    	        break;
    	    }

        handle_stdin(fs, buf, &err);
        if (buf){
        	free(buf);
        }
    } while (terminator > 0);

    /* Stop allowing blocking reads on streams (to prevent unmount from
       blocking forever) and lazy-unmount the filesystem.  For complete
       correctness, this should disallow new image opens, wait for existing
       image fds to close, disallow new stream opens and blocking reads,
       then lazy unmount. */
    image_close(fs->disk);
    if (fs->memory != NULL) {
        image_close(fs->memory);
    }
    _vmnetfs_fuse_terminate(fs->fuse);
    return FALSE;
}

static gboolean shutdown_callback(void *data)
{
    struct vmnetfs *fs = data;

    g_main_loop_quit(fs->glib_loop);
    return FALSE;
}

static void child(FILE *pipe)
{
    struct vmnetfs *fs;
    GThread *loop_thread = NULL;
    GIOChannel *chan;
    GIOChannel *chan_out;
    GIOFlags flags;
    int argc;
    char **argv;
    int arg = 0;
    int images;
    char *username;
    char *password;
    GError *err = NULL;

    /* Initialize */
    if (!g_thread_supported()) {
        g_thread_init(NULL);
    }
    if (!_vmnetfs_transport_init()) {
        fprintf(pipe, "Could not initialize transport\n");
        fclose(pipe);
        return;
    }

    /* Read arguments */
    chan = g_io_channel_unix_new(0);
    chan_out = g_io_channel_unix_new(1);
#ifdef DEBUG_STAND_ALONE
    // krha
    GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_add(args, "");                                  // user id
    g_ptr_array_add(args, "");                                  // password
    g_ptr_array_add(args, "disk");                                        // dummy url
    g_ptr_array_add(args, "/home/krha/cloudlet/image/ubuntu-12.04.1-server-i386/precise.raw");       // base path
    g_ptr_array_add(args, "/home/krha/cloudlet/src/server/tmp/img");
    g_ptr_array_add(args, "1082595:1,1082610:1,1082552:1,1082426:1,55357:1,1082547:1,1082415:1,1082562:1,1082507:1,1082498:1,1082539:1,1082609:1,1082475:1,1082621:1,1082446:1,1082560:1,1082499:1,1082607:1,1082471:1,1082582:1,1082479:1,1082441:1,1082554:1,1082494:1,1082521:1,1082456:1,1082544:1,1082548:1,557563:1,1058408:1,557555:1,1082488:1,1082587:1,1082616:1,1082382:1,1082599:1,1082451:1,1082512:1,1082400:1,1082452:1,1082438:1,1082604:1,1082381:1,1082470:1,1082504:1,1082522:1,1082432:1,1082370:1,557562:1,1048970:1,1082598:1,1082540:1,1082519:1,1082557:1,1082454:1,557553:1,1082601:1,1082619:1,102225:1,1082490:1,1082405:1,1082463:1,1082514:1,1082388:1,1082525:1,1082480:1,1082389:1,1082453:1,1082529:1,1082458:1,1082528:1,1082379:1,1082520:1,1082465:1,1082513:1,1082579:1,1082485:1,1082434:1,1082538:1,1082517:1,1082567:1,1082589:1,1082594:1,871:1,557564:1,1082369:1,1082390:1,1082584:1,1082496:1,1082461:1,1082561:1,1082464:1,1082443:1,1082559:1,1082531:1,1082527:1,1082401:1,1082391:1,1082615:1,1082511:1,1082399:1,1082571:1,1082583:1,1082575:1,1082469:1,1082608:1,1082455:1,1082537:1,1082398:1,1082377:1,1082497:1,1082468:1,1082613:1,1082410:1,1082597:1,1082533:1,1082411:1,1082508:1,1082506:1,1082566:1,1082440:1,1082510:1,1082555:1,1082368:1,1082603:1,1082476:1,1082486:1,1082457:1,1082378:1,1082515:1,1082524:1,1082386:1,1082588:1,1082396:1,1082606:1,1082414:1,1082493:1,1082576:1,1082565:1,1082413:1,1082416:1,1082467:1,1082549:1,1082591:1,1082439:1,1082618:1,1082623:1,1082412:1,1082518:1,1082532:1,1082393:1,1082422:1,1082546:1,1082586:1,1082509:1,1082605:1,1082376:1,557558:1,1082395:1,1082474:1,1082580:1,1082592:1,1082466:1,1082481:1,1082596:1,1082373:1,1082516:1,1082477:1,1082472:1,1082430:1,1082536:1,1082581:1,1082545:1,1082492:1,1082489:1,1082487:1,1082614:1,1082553:1,1082543:1,1082425:1,1082374:1,1082462:1,746:1,557554:1,1082444:1,1082459:1,1082478:1,557557:1,1082442:1,1082375:1,1082460:1,1082542:1,1082436:1,1082427:1,1082433:1,1082577:1,557565:1,1048833:1,1082435:1,1082593:1,1082551:1,1082421:1,1082449:1,1082574:1,1082448:1,1082371:1,557561:1,1048898:1,1082612:1,1082437:1,1082620:1,557566:1,1082417:1,1082491:1,1082556:1,1082419:1,1082402:1,1082585:1,1082447:1,1082550:1,1082611:1,1082404:1,1082418:1,1082535:1,1082383:1,1082578:1,1082563:1,1082573:1,1082408:1,1082501:1,1082534:1,1082445:1,1082602:1,1082523:1,1082502:1,1082407:1,1082484:1,1082423:1,1082403:1,1082450:1,1082590:1,1082424:1,1082431:1,1082428:1,1082564:1,1082495:1,1082572:1,1082503:1,1082569:1,1082394:1,1082392:1,257:1,557560:1,737:1,557556:1,1082409:1,1082526:1,1082380:1,1082568:1,1082617:1,1082473:1,1082384:1,1082397:1,1082505:1,1082530:1,557559:1,1048848:1,1082483:1,1082622:1,1082558:1,1082570:1,1082387:1,1082541:1,1082385:1,1082500:1,1082406:1,1082372:1,1082600:1,1082429:1,1082420:1,1082482:1");
    g_ptr_array_add(args, "8589934592");                // size of base (maximum for raw case)
    g_ptr_array_add(args, "0");                                 // segment size
    g_ptr_array_add(args, "4096");                              // chunk size: 131072 --> 4096

	g_ptr_array_add(args, "memory");
	g_ptr_array_add(args, "/home/krha/cloudlet/image/ubuntu-12.04.1-server-i386/precise.base-mem");       // base path
	g_ptr_array_add(args, "/home/krha/cloudlet/src/server/tmp/mem");
	g_ptr_array_add(args, "0:1,1:1,6109:1,6114:1,6115:1,6118:1,6121:1,6123:1,6131:1,6132:1,6133:1,6136:1,6148:1,6151:1,6174:1,6175:1,6180:1,6206:1,6208:1,6210:1,6211:1,6312:1,6428:1,6429:1,6430:1,6431:1,6438:1,6463:1,6464:1,6465:1,6481:1,6482:1,6486:1,6487:1,6601:1,6602:1,6603:1,6604:1,6605:1,6606:1,6608:1,6609:1,6613:1,6615:1,6619:1,6623:1,6625:1,6628:1,216074:1,216094:1,216107:1,216131:1,216155:1,216168:1,216169:1,216194:1,216195:1,216196:1,216216:1,216221:1,216223:1,216233:1,216245:1,216246:1,216249:1,216311:1,217092:1,217104:1,217106:1,217117:1,217138:1,217165:1,217186:1,217187:1,217189:1,217190:1,217192:1,217300:1,217303:1,217305:1,217339:1,217340:1,217341:1,217342:1,217344:1,217345:1,217354:1,217367:1,217369:1,217450:1,217478:1,217484:1,217526:1,217549:1,217554:1,217555:1,217615:1,217622:1,217623:1,217625:1,217631:1,217645:1,217687:1,217688:1,217691:1,217721:1,217755:1,217793:1,217812:1,217818:1,217819:1,217827:1,217828:1,217829:1,217930:1,217936:1,217937:1,217945:1,217972:1,217973:1,217998:1,218015:1,218041:1,218074:1,218081:1,218092:1,218095:1,218097:1,218104:1,218111:1,218315:1,218320:1,218396:1,218411:1,218422:1,218438:1,218442:1,218458:1,218459:1,218460:1,218749:1,218750:1,218754:1,218758:1,218770:1,218786:1,218794:1,218822:1,218834:1,218847:1,218848:1,218849:1,218851:1,218853:1,218854:1,218860:1,218898:1,218899:1,218914:1,218916:1,218932:1,218955:1,218957:1,218960:1,218966:1,218979:1,218985:1,218990:1,219000:1,219002:1,219014:1,219015:1,219033:1,219037:1,219046:1,219048:1,219090:1,219091:1,219125:1,219135:1,219141:1,219142:1,219173:1,219178:1,219179:1,219182:1,219183:1,219186:1,219187:1,219196:1,219198:1,219202:1,219203:1,219204:1,219205:1,219211:1,219213:1,219219:1,219221:1,219222:1,219226:1,219228:1,219230:1,219231:1,219236:1,219244:1,219245:1,219246:1,219281:1,219282:1,219283:1,219751:1,219822:1,219823:1,219824:1,219825:1,219826:1,219828:1,219830:1,219837:1,219839:1,219842:1,219843:1,219844:1,219845:1,219846:1,219847:1,219848:1,219849:1,219858:1,219859:1,219860:1,219861:1,219863:1,219866:1,219867:1,219868:1,219869:1,219873:1,219877:1,219884:1,219886:1,219887:1,219888:1,219889:1,219890:1,219891:1,219892:1,219893:1,219897:1,219898:1,219899:1,219900:1,219901:1,219902:1,219903:1,219904:1,219905:1,219906:1,219907:1,219908:1,219909:1,219910:1,219911:1,219912:1,219913:1,219922:1,219923:1,219926:1,219995:1,220000:1,220001:1,220033:1,220034:1,220044:1,220048:1,220050:1,220052:1,220056:1,220057:1,220060:1,220100:1,220102:1,220119:1,220124:1,220126:1,220127:1,220128:1,220129:1,220130:1,220131:1,220132:1,220133:1,220154:1,220157:1,220159:1,220160:1,220162:1,220163:1,220166:1,220168:1,220170:1,220172:1,220173:1,220175:1,220177:1,220186:1,220282:1,220283:1,220286:1,220287:1,220289:1,220290:1,220295:1,220297:1,220298:1,220313:1,220315:1,220319:1,220321:1,220322:1,220324:1,220326:1,220330:1,220331:1,220333:1,220334:1,220335:1,220336:1,220339:1,220341:1,220343:1,220345:1,220351:1,220353:1,220355:1,220357:1,220359:1,220361:1,220367:1,220368:1,220369:1,220370:1,220371:1,220376:1,220382:1,220411:1,220413:1,220438:1,220447:1,220450:1,220464:1,220465:1,220474:1,220496:1,220497:1,220534:1,220535:1,220586:1,220588:1,220589:1,220611:1,220613:1,220614:1,220616:1,220617:1,220623:1,220654:1,220669:1,220701:1,220704:1,220707:1,220733:1,220872:1,221380:1,221486:1,221499:1,222414:1,222415:1,222418:1,222419:1,222420:1,222422:1,222423:1,222424:1,222425:1,222426:1,222427:1,222428:1,222429:1,222430:1,222431:1,222432:1,222433:1,222434:1,222435:1,222436:1,222437:1,222438:1,222439:1,222440:1,222441:1,222442:1,222443:1,222444:1,222445:1,222449:1,222471:1,222481:1,222523:1,222735:1,223235:1,223238:1,223239:1,223240:1,223241:1,223249:1,223262:1,223263:1,223264:1,223265:1,223266:1,223267:1,223268:1,223269:1,223270:1,223271:1,223272:1,223273:1,223274:1,223275:1,223276:1,223277:1,223278:1,223279:1,223280:1,223281:1,223282:1,223283:1,223284:1,223285:1,223286:1,223287:1,223288:1,223289:1,223290:1,223291:1,223292:1,223293:1,223294:1,223295:1,223296:1,223297:1,223298:1,223299:1,223300:1,223301:1,223302:1,223303:1,223304:1,223305:1,223306:1,223307:1,223308:1,223309:1,223310:1,223311:1,223312:1,223313:1,223314:1,223315:1,223316:1,223317:1,223318:1,223319:1,223320:1,223321:1,223322:1,223323:1,223324:1,223325:1,223326:1,223327:1,223328:1,223329:1,223330:1,223331:1,223332:1,223333:1,223334:1,223335:1,223336:1,223337:1,223338:1,223339:1,223340:1,223341:1,223342:1,223343:1,223344:1,223345:1,223346:1,223347:1,223348:1,223349:1,223350:1,223351:1,223352:1,223353:1,223354:1,223355:1,223356:1,223357:1,223358:1,223359:1,223360:1,223361:1,223362:1,223363:1,223364:1,223365:1,223366:1,223367:1,223368:1,223369:1,223370:1,223371:1,223372:1,223373:1,223374:1,223375:1,223376:1,223377:1,223378:1,223379:1,223380:1,223381:1,223382:1,223383:1,223384:1,223385:1,223386:1,223387:1,223388:1,223389:1,223390:1,223391:1,223392:1,223393:1,223394:1,223395:1,223396:1,223397:1,223398:1,223399:1,223400:1,223401:1,223402:1,223403:1,223404:1,223405:1,223406:1,223407:1,223408:1,223409:1,223410:1,223411:1,223412:1,223413:1,223414:1,223415:1,223416:1,223417:1,223421:1,223423:1,223431:1,223432:1,223433:1,223434:1,223441:1,223446:1,223447:1,223448:1,223449:1,223450:1,223451:1,223452:1,223453:1,223454:1,223455:1,223456:1,223457:1,223458:1,223459:1,223460:1,223461:1,223462:1,223463:1,223464:1,223465:1,223466:1,223467:1,223468:1,223469:1,223470:1,223471:1,223472:1,223473:1,223474:1,223475:1,223476:1,223477:1,223478:1,223479:1,223480:1,223481:1,223482:1,223483:1,223484:1,223485:1,223486:1,223487:1,223488:1,223489:1,223490:1,223491:1,223492:1,223493:1,223494:1,223495:1,223496:1,223497:1,223498:1,223499:1,223500:1,223501:1,223502:1,223503:1,223504:1,223505:1,223506:1,223507:1,223508:1,223509:1,223510:1,223511:1,223512:1,223513:1,223514:1,223515:1,223516:1,223517:1,223554:1,223558:1,223559:1,223560:1,223561:1,223562:1,223563:1,223564:1,223565:1,223566:1,223567:1,223574:1,223575:1,223576:1,223577:1,223580:1,223581:1,223582:1,223584:1,223588:1,223598:1,223602:1,223603:1,223604:1,223605:1,223606:1,223607:1,223608:1,223609:1,223614:1,223615:1,223616:1,223617:1,223620:1,223622:1,223623:1,223624:1,223625:1,223629:1,223636:1,223637:1,223638:1,223639:1,223640:1,223641:1,223642:1,223643:1,223644:1,223645:1,223646:1,223671:1,223673:1,223674:1,223697:1,223698:1,223699:1,223702:1,223710:1,223711:1,223712:1,223713:1,223722:1,223723:1,223724:1,223734:1,223747:1,223804:1,223843:1,223844:1,223845:1,223846:1,223849:1,223850:1,223851:1,223852:1,223853:1,223854:1,223855:1,223856:1,223857:1,223858:1,223859:1,223860:1,223861:1,223862:1,223863:1,223864:1,223865:1,223866:1,223867:1,223868:1,223869:1,223870:1,224259:1,224269:1,224277:1,224307:1,224312:1,224326:1,224333:1,224335:1,224345:1,224346:1,224359:1,224360:1,224362:1,224368:1,224374:1,224379:1,224409:1,224413:1,224437:1,224471:1,224477:1,224483:1,224507:1,224508:1,224521:1,224526:1,224540:1,224561:1,224569:1,224581:1,224586:1,224636:1,224650:1,224651:1,224673:1,224686:1,224699:1,224700:1,224701:1,224703:1,224704:1,224716:1,224717:1,224720:1,224731:1,224732:1,224733:1,224740:1,224751:1,224755:1,224766:1,224767:1,224793:1,224794:1,224800:1,224808:1,224809:1,224885:1,224898:1,224942:1,224957:1,224969:1,224999:1,225003:1,225013:1,225027:1,225029:1,225030:1,225031:1,225044:1,225046:1,225048:1,225054:1,225055:1,225071:1,225100:1,225111:1,225151:1,225153:1,225158:1,225159:1,225174:1,225175:1,225195:1,225206:1,225209:1,225234:1,225236:1,225863:1,225864:1,225878:1,225883:1,225884:1,225931:1,225940:1,225941:1,226061:1,226097:1,226116:1,226118:1,226122:1,226174:1,226178:1,226180:1,226210:1,226242:1,226243:1,226244:1,226245:1,226247:1,226248:1,226249:1,226252:1,226253:1,226256:1,226257:1,226258:1,226259:1,226261:1,226262:1,226263:1,226266:1,226267:1,226270:1,226271:1,226272:1,226273:1,226275:1,226276:1,226277:1,226280:1,226281:1,226284:1,226285:1,226286:1,226287:1,226289:1,226290:1,226291:1,226295:1,226299:1,226300:1,226301:1,227991:1,227992:1,227999:1,228000:1,228001:1,228002:1,228003:1,228004:1,228005:1,228006:1,228009:1,228011:1,228012:1,228013:1,228014:1,228015:1,228016:1,228020:1,228022:1,228024:1,228025:1,228026:1,228027:1,228032:1,228047:1,228048:1,228049:1,228050:1,228051:1,228052:1,228055:1,228056:1,228057:1,228058:1,228059:1,228060:1,228061:1,228062:1,228127:1,228128:1,228129:1,228135:1,228136:1,228137:1,228138:1,228139:1,228140:1,228141:1,228142:1,228143:1,228144:1,228145:1,228146:1,228147:1,228148:1,228149:1,228150:1,228151:1,228152:1,228153:1,228154:1,228156:1,228157:1,228158:1,228159:1,228160:1,228161:1,228162:1,228163:1,228164:1,228165:1,228166:1,228167:1,228168:1,228169:1,228170:1,228171:1,228172:1,228173:1,228174:1,228175:1,228176:1,228178:1,228179:1,228180:1,228182:1,228183:1,228184:1,228185:1,228186:1,228187:1,228188:1,228189:1,228190:1,228197:1,228198:1,228223:1,228241:1,228242:1,228249:1,228250:1,228260:1,228263:1,228264:1,228266:1,228271:1,228272:1,228273:1,228274:1,228290:1,228335:1,228344:1,228345:1,228347:1,228348:1,228350:1,228351:1,233575:1,233585:1,233592:1,233624:1,233714:1,233745:1,233753:1,233761:1,233779:1,233789:1,234542:1,234543:1,234544:1,234545:1,234553:1,234561:1,234562:1,234563:1,234564:1,234565:1,234574:1,234575:1,234576:1,234577:1,234578:1,234610:1,234611:1,234612:1,234613:1,234622:1,234623:1,234624:1,234625:1,234660:1,234666:1,234668:1,234690:1,234691:1,234692:1,234693:1,234701:1,234706:1,234707:1,234708:1,234709:1,234713:1,234716:1,234719:1,234721:1,234775:1,234866:1,234885:1,234891:1,234931:1,234962:1,234966:1,234973:1,234979:1,234982:1,234987:1,234992:1,235030:1,235036:1,235040:1,235054:1,235057:1,235066:1,235071:1,235076:1,235108:1,235131:1,235159:1,235174:1,235177:1,235186:1,235217:1,235218:1,235221:1,235226:1,235240:1,235241:1,235255:1,235309:1,235348:1,235390:1,235395:1,235411:1,235423:1,235442:1,235443:1,235444:1,235445:1,235461:1,235465:1,235560:1,235570:1,235571:1,235597:1,235641:1,235654:1,235655:1,235662:1,235663:1,235664:1,235665:1,235666:1,235667:1,235674:1,235678:1,235737:1,235859:1,235869:1,235874:1,235882:1,235883:1,235884:1,235885:1,235902:1,235903:1,235904:1,235922:1,235924:1,235938:1,235939:1,235940:1,235941:1,235943:1,235949:1,235974:1,235975:1,235982:1,235983:1,235984:1,235985:1,236074:1,236098:1,236099:1,236100:1,236101:1,236114:1,236115:1,236116:1,236117:1,236121:1,236122:1,236123:1,236125:1,236126:1,236127:1,236152:1,236174:1,236175:1,236179:1,236183:1,236185:1,236193:1,236194:1,236195:1,236196:1,236197:1,236202:1,236203:1,236204:1,236205:1,236206:1,236207:1,236220:1,236237:1,236245:1,236250:1,236251:1,236252:1,236253:1,236260:1,236261:1,236370:1,236371:1,236372:1,236373:1,236377:1,236418:1,236419:1,236420:1,236421:1,236424:1,236479:1,236518:1,236519:1,236520:1,236521:1,236522:1,236629:1,236667:1,236678:1,236679:1,236680:1,236681:1,236684:1,236694:1,236696:1,236946:1,236950:1,236951:1,236952:1,236953:1,236966:1,236967:1,236968:1,237073:1,237132:1,237227:1,237229:1,237230:1,237231:1,237232:1,237233:1,237262:1,237263:1,237264:1,237265:1,237332:1,237333:1,237334:1,237335:1,237336:1,237337:1,237485:1,237493:1,237828:1,237829:1,237894:1,237895:1,237896:1,237897:1,237949:1,238002:1,238034:1,238035:1,238036:1,238037:1,238038:1,238039:1,238040:1,238048:1,238054:1,238055:1,238056:1,238057:1,238058:1,238059:1,238060:1,238061:1,238064:1,238083:1,238092:1,238094:1,238113:1,238131:1,238137:1,238138:1,238139:1,238140:1,238141:1,238156:1,238174:1,238175:1,238176:1,238177:1,238286:1,238302:1,238305:1,238413:1,238542:1,238543:1,238544:1,238545:1,238549:1,238555:1,238597:1,238618:1,238619:1,238620:1,238621:1,238630:1,238631:1,238632:1,238633:1,238706:1,238707:1,238708:1,238709:1,238830:1,238858:1,238870:1,239034:1,239035:1,239036:1,239038:1,239039:1,239040:1,239041:1,239070:1,239071:1,239072:1,239073:1,239105:1,239144:1,239148:1,239210:1,239211:1,239212:1,239213:1,239217:1,239230:1,239231:1,239232:1,239233:1,239265:1,239274:1,239275:1,239277:1,239298:1,239302:1,239303:1,239304:1,239305:1,239346:1,239347:1,239348:1,239349:1,239353:1,239354:1,239355:1,239356:1,239357:1,239366:1,239367:1,239368:1,239369:1,239442:1,239443:1,239451:1,239454:1,239455:1,239456:1,239457:1,239654:1,239676:1,239701:1,239771:1,239778:1,239779:1,239780:1,239781:1,239791:1,239807:1,239846:1,239848:1,239849:1,239872:1,240031:1,240040:1,240136:1,240137:1,240254:1,240255:1,240256:1,240257:1,240278:1,240301:1,240536:1,240541:1,240546:1,240602:1,240603:1,240604:1,240605:1,240606:1,240607:1,240626:1,240634:1,240635:1,240638:1,240639:1,240640:1,240641:1,240643:1,240644:1,240650:1,240651:1,240654:1,240655:1,240656:1,240657:1,240677:1,240679:1,240685:1,240693:1,240741:1,240762:1,240763:1,240806:1,240807:1,240808:1,240809:1,240813:1,240822:1,240846:1,240847:1,240848:1,240849:1,240866:1,240867:1,240868:1,240869:1,240870:1,240878:1,240881:1,240937:1,240982:1,240983:1,240984:1,240985:1,241008:1,241022:1,241035:1,241037:1,241040:1,241041:1,241054:1,241063:1,241087:1,241106:1,241107:1,241108:1,241109:1,241147:1,241190:1,241191:1,241192:1,241203:1,241250:1,241251:1,241252:1,241253:1,241254:1,241255:1,241279:1,241289:1,241293:1,241294:1,241304:1,241325:1,241330:1,241331:1,241332:1,241333:1,241356:1,241362:1,241363:1,241373:1,241386:1,241387:1,241388:1,241389:1,241406:1,241411:1,241437:1,241438:1,241441:1,241454:1,241455:1,241460:1,241511:1,241512:1,241552:1,241600:1,241601:1,241602:1,241612:1,241614:1,241619:1,241628:1,241631:1,241638:1,241639:1,241640:1,241641:1,241642:1,241643:1,241644:1,241654:1,241655:1,242175:1,242557:1,242569:1,242584:1,242612:1,242619:1,242626:1,242627:1,242629:1,242638:1,242645:1,242646:1,242662:1,248987:1,249248:1,249266:1,249473:1,249520:1,249741:1,250895:1,251107:1,252001:1,252179:1,252298:1,252385:1,252392:1,252393:1,252396:1,254381:1,260207:1,261327:1,261416:1,261453:1,261690:1,261762:1,261767:1,262018:1,262020:1,262029:1,262031:1,262148:1,262149:1,266243:1,266244:1,266245:1,266246:1,266247:1,266248:1,266249:1,266250:1,266251:1,266252:1,266253:1,266254:1,266255:1,266256:1,266257:1,266258:1,266259:1,266260:1,266261:1,266262:1,266263:1,266264:1,266265:1,266266:1,266267:1,266268:1,266269:1,266270:1,266271:1,266272:1,266273:1,266274:1,266275:1,266276:1,266277:1,266280:1,266281:1,266282:1,266283:1,266284:1,266285:1,266286:1,266287:1,266288:1,266289:1,266290:1,266291:1,266292:1,266293:1,266294:1,266295:1,266296:1,266297:1,266298:1,266299:1,266300:1,266301:1,266302:1,266303:1,266304:1,266305:1,266306:1,266307:1,266308:1,266309:1,266310:1,266311:1,266312:1,266313:1,266314:1,266315:1,266316:1,266317:1,266318:1,266319:1,266320:1,266321:1,266322:1,266323:1,266324:1,266326:1,266327:1,266328:1,266329:1,266330:1,266331:1,266332:1,266333:1,266334:1,266335:1,266336:1,266337:1,266338:1,266339:1,266340:1,266341:1,266342:1,266343:1,266344:1,266345:1,266346:1,266347:1");
	g_ptr_array_add(args, "1090961551");                // size of base (maximum for raw case)
	g_ptr_array_add(args, "0");                                 // segment size
	g_ptr_array_add(args, "4096");                              // chunk size: 131072 --> 4096
    argv = (char **) g_ptr_array_free(args, FALSE);
#else
    argv = get_arguments(chan, &err);
#endif

    if (argv == NULL) {
        fprintf(pipe, "%s\n", err->message);
        g_clear_error(&err);
        fclose(pipe);
        return;
    }

    /* Check argc */
    argc = g_strv_length(argv);
    images = (argc - 2) / IMAGE_ARG_COUNT;
    if (argc % IMAGE_ARG_COUNT != 2 || images < 1 || images > 2) {
        fprintf(pipe, "Incorrect argument count\n");
        fclose(pipe);
        return;
    }

    /* Get initial arguments */
    username = argv[arg++];
    if (!username[0]) {
        username = NULL;
    }
    password = argv[arg++];
    if (!password[0]) {
        password = NULL;
    }

    /* Set up disk */
    fs = g_slice_new0(struct vmnetfs);
    fs->disk = image_new(argv + arg, username, password, &err);
    if (err) {
        fprintf(pipe, "%s\n", err->message);
        goto out;
    }
    arg += IMAGE_ARG_COUNT;

    /* Set up memory */
    if (images > 1) {
        fs->memory = image_new(argv + arg, username, password, &err);
        if (err) {
            fprintf(pipe, "%s\n", err->message);
            goto out;
        }
        arg += IMAGE_ARG_COUNT;
    }

    /* Set up fuse */
    fs->fuse = _vmnetfs_fuse_new(fs, &err);
    if (err) {
        fprintf(pipe, "%s\n", err->message);
        goto out;
    }

    /* Start main loop thread */
    loop_thread = g_thread_create(glib_loop_thread, fs, TRUE, &err);
    if (err) {
        fprintf(pipe, "%s\n", err->message);
        goto out;
    }

    /* Add watch for stdin being closed */
    flags = g_io_channel_get_flags(chan);
    g_io_channel_set_flags(chan, flags | G_IO_FLAG_NONBLOCK, &err);
    if (err) {
        fprintf(pipe, "%s\n", err->message);
        g_io_channel_unref(chan);
        goto out;
    }
    g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, read_stdin, fs);

    /* Started successfully.  Send the mountpoint back to the parent and
       run FUSE event loop until the filesystem is unmounted. */
    fprintf(pipe, "%s\n", fs->fuse->mountpoint);
    fflush(pipe);
    //fclose(pipe);
    //pipe = NULL;
    _vmnetfs_fuse_run(fs->fuse);

out:
    /* Shut down */
    if (err != NULL) {
        g_clear_error(&err);
    }
    if (pipe != NULL) {
        fclose(pipe);
    }
    if (loop_thread != NULL) {
        g_idle_add(shutdown_callback, fs);
        g_thread_join(loop_thread);
    }
    _vmnetfs_fuse_free(fs->fuse);
    image_free(fs->disk);
    image_free(fs->memory);
    g_slice_free(struct vmnetfs, fs);
    g_strfreev(argv);
    g_io_channel_unref(chan);
}

static void setsignal(int signum, void (*handler)(int))
{
    const struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags = SA_RESTART,
    };

    sigaction(signum, &sa, NULL);
}

int main(int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
    int pipes[2];
    FILE *pipe_fh;
    pid_t pid;

    setsignal(SIGINT, SIG_IGN);

    if (pipe(pipes)) {
        fprintf(stderr, "Could not create pipes\n");
        return 1;
    }

	// Do not close communication between parent
	// We'll use this communication to update status of overlay
    child(stdout);
    return 0;

    /*
    pid = fork();
    if (pid) {
        // Parent
        char buf[256];
        int status;
        pid_t exited;

        pipe_fh = fdopen(pipes[0], "r");
        close(pipes[1]);

        // Read possible error status from child
        buf[0] = 0;
        fgets(buf, sizeof(buf), pipe_fh);
        if (ferror(pipe_fh)) {
            fprintf(stderr, "Error reading status from vmnetfs\n");
            return 1;
        }
        if (buf[0] != 0 && buf[0] != '\n') {
            fprintf(stderr, "%s", buf);
            return 1;
        }

        // See if it exited
        exited = waitpid(pid, &status, WNOHANG);
        if (exited == -1) {
            fprintf(stderr, "Error reading exit status from vmnetfs\n");
            return 1;
        } else if (exited && WIFSIGNALED(status)) {
            fprintf(stderr, "vmnetfs died on signal %d\n", WTERMSIG(status));
            return 1;
        } else if (exited) {
            fprintf(stderr, "vmnetfs died with exit status %d\n",
                    WEXITSTATUS(status));
            return 1;
        }

        // Print mountpoint and exit
        buf[0] = 0;
        fgets(buf, sizeof(buf), pipe_fh);
        if (ferror(pipe_fh)) {
            fprintf(stderr, "Error reading mountpoint from vmnetfs\n");
            return 1;
        }
//        printf("%s", buf);
        return 0;

    } else {

        // Child
        pipe_fh = fdopen(pipes[1], "w");
        close(pipes[0]);

        // Ensure the grandparent doesn't block reading our output
        close(1);
        close(2);
        open("/dev/null", O_WRONLY);
        open("/dev/null", O_WRONLY);
        child(pipe_fh);
        return 0;
    }
    */
}
