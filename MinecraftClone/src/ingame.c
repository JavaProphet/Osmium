/*
 * ingame.c
 *
 *  Created on: Mar 6, 2016
 *      Author: root
 */

#include "ingame.h"
#include "network.h"
#include "world.h"
#include "entity.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include "globals.h"
#include "render.h"
#include "gui.h"
#include "block.h"
#include <errno.h>

int running = 0;
struct vao skybox;

void loadIngame() {
	gs.moveSpeed = 0.1;
	struct vertex vts[676];
	int vi = 0;
	for (int x1 = -384; x1 <= 384; x1 += 64) {
		for (int x2 = -384; x2 <= 384; x2 += 64) {
			virtVertex3f(&vts[vi++], x1, 16., x2);
			virtVertex3f(&vts[vi++], x1 + 64., 16., x2);
			virtVertex3f(&vts[vi++], x1 + 64., 16., x2 + 64.);
			virtVertex3f(&vts[vi++], x1, 16., x2 + 64.);
		}
	}
	createVAO(vts, 676, &skybox, 0, 0);
}

void ingame_keyboardCallback(int key, int scancode, int action, int mods) {
	if (gs.inMenu) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			gs.inMenu = 0;
		}
	} else {
		if (key == GLFW_KEY_W) {
			gs.moveForward = action == GLFW_PRESS || action == GLFW_REPEAT;
		} else if (key == GLFW_KEY_S) {
			gs.moveBackward = action == GLFW_PRESS || action == GLFW_REPEAT;
		} else if (key == GLFW_KEY_A) {
			gs.moveLeft = action == GLFW_PRESS || action == GLFW_REPEAT;
		} else if (key == GLFW_KEY_D) {
			gs.moveRight = action == GLFW_PRESS || action == GLFW_REPEAT;
		} else if (key == GLFW_KEY_SPACE) {
			gs.jumping = action == GLFW_PRESS || action == GLFW_REPEAT;
		} else if (key == GLFW_KEY_LEFT_CONTROL) {
			gs.sprinting = action == GLFW_PRESS || action == GLFW_REPEAT;
		} else if (key == GLFW_KEY_LEFT_SHIFT) {
			gs.crouching = action == GLFW_PRESS || action == GLFW_REPEAT;
		} else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			gs.inMenu = 1;
			gs.crouching = 0;
			gs.sprinting = 0;
		}
	}
}

int moc = 0;
double lmx = 0.;
double lmy = 0.;

void ingame_mouseMotionCallback(double x, double y) {
	if (gs.player != NULL && spawnedIn && hasMouse && !gs.inMenu) {
		if (!moc) {
			moc = 1;
			claimMouse();
		}
		int dx = lmx - x;
		int dy = lmy - y;
		gs.player->pitch -= dy * 0.1;
		if (gs.player->pitch > 89.9) gs.player->pitch = 89.9;
		if (gs.player->pitch < -89.9) gs.player->pitch = -89.9;
		gs.player->yaw -= dx * 0.1;
	} else if (moc) {
		moc = 0;
		unclaimMouse();
	}
	lmx = x;
	lmy = y;
}

void ingame_tick() {
	if (!running) return;
	for (size_t i = 0; i < gs.world->entity_count; i++) {
		struct entity* ent = gs.world->entities[i];
		if (ent != NULL) {
			ent->lx = gs.world->entities[i]->x;
			ent->ly = gs.world->entities[i]->y;
			ent->lz = gs.world->entities[i]->z;
			ent->lyaw = gs.world->entities[i]->yaw;
			ent->lpitch = gs.world->entities[i]->pitch;
			if (fabs(ent->motX) < 0.005) ent->motX = 0.;
			if (fabs(ent->motY) < 0.005) ent->motY = 0.;
			if (fabs(ent->motZ) < 0.005) ent->motZ = 0.;
		}
	}
	if (gs.jumping) {
		//if inwater
		//motY += 0.03999999910593033
		//else
		if (gs.player->onGround && gs.jumpTicks <= 0) {
			gs.player->motY = 0.42;
			//TODO: speed potion
			if (gs.sprinting) {
				float v1 = gs.player->yaw * .017453292;
				gs.player->motX -= sin(v1) * 0.2;
				gs.player->motZ += cos(v1) * 0.2;
			}
			gs.jumpTicks = 10;
		} else if (gs.jumpTicks > 0) gs.jumpTicks--;
	} else if (gs.jumpTicks > 0) gs.jumpTicks--;
	if (gs.player->health <= 0.) {
		gs.moveForward = 0;
		gs.moveBackward = 0;
		gs.moveLeft = 0;
		gs.moveRight = 0;
	}
	float moveForward = gs.moveForward ? 1. : 0.;
	if (gs.moveBackward) moveForward -= 1;
	//printf("move %f %i %i\n", moveForward, gs.moveForward, gs.moveBackward);
	float moveStrafe = gs.moveLeft ? -1. : 0.;
	if (gs.moveRight) moveStrafe += 1;
	if (gs.flying && gs.riding == NULL) {
		//TODO: flight
	} else {
		//TODO if in water and not flying

		//TODO else if in lava and not flying

		//ELSE
		moveStrafe *= 0.98;
		moveForward *= 0.98;
		if (gs.usingItem) {
			moveForward *= .2;
			moveStrafe *= .2;
		}
		double v3 = .91;
		if (gs.player->onGround) {
			v3 *= 0.6;
			//TODO get slipperniss of block below, multiply by .91 set to v3
		}
		float v4 = .16277136 / (v3 * v3 * v3);
		float v5 = .02;
		if (gs.player->onGround) {
			v5 = gs.moveSpeed * (gs.sprinting ? 1.3 : 1.) * v4;
		} else {
			v5 = .02;
		}
		v4 = moveStrafe * moveStrafe + moveForward * moveForward;
		if (v4 > 0.0001) {
			v4 = sqrt(v4);
			if (v4 < 1.) v4 = 1.;
			float ff = v5 / v4;
			float f5 = sin(-gs.player->yaw * 0.017453292);
			float f6 = cos(-gs.player->yaw * 0.017453292);
			float nmx = moveStrafe * ff * f6 - moveForward * ff * f5;
			float nmz = moveForward * ff * f6 + moveStrafe * ff * f5;
			gs.player->motX -= nmx;
			gs.player->motZ += nmz;
		}
		v3 = .91;
		if (gs.player->onGround) {
			v3 *= 0.6;
			//TODO get slipperniss of block below, multiply by .91 set to v3
		}
		//
		//TODO: if in web
		if (gs.player->onGround && gs.crouching) {		//TODO set crouching
			//TODO: crouch bounding
		}
		struct boundingbox obb;
		obb.minX = gs.player->x - 0.3;
		obb.maxX = gs.player->x + 0.3;
		obb.minZ = gs.player->z - 0.3;
		obb.maxZ = gs.player->z + 0.3;
		obb.minY = gs.player->y;
		obb.maxY = gs.player->y + 1.8;
		if (gs.player->motX < 0.) {
			obb.minX += gs.player->motX;
		} else {
			obb.maxX += gs.player->motX;
		}
		if (gs.player->motY < 0.) {
			obb.minY += gs.player->motY;
		} else {
			obb.maxY += gs.player->motY;
		}
		if (gs.player->motZ < 0.) {
			obb.minZ += gs.player->motZ;
		} else {
			obb.maxZ += gs.player->motZ;
		}
		struct boundingbox pbb;
		pbb.minX = gs.player->x - 0.3;
		pbb.maxX = gs.player->x + 0.3;
		pbb.minZ = gs.player->z - 0.3;
		pbb.maxZ = gs.player->z + 0.3;
		pbb.minY = gs.player->y;
		pbb.maxY = gs.player->y + 1.8;
		double ny = gs.player->motY;
		for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
			for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
				for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
					block b = getBlockWorld(gs.world, x, y, z);
					if (b > 0 && doesBlockCollide(b)) {
						struct boundingbox* bb = getBlockCollision(b);
						if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
							if (pbb.maxX > bb->minX + x && pbb.minX < bb->maxX + x && pbb.maxZ > bb->minZ + z && pbb.minZ < bb->maxZ + z) {
								double t;
								if (ny > 0. && pbb.maxY <= bb->minY + y) {
									t = bb->minY + y - pbb.maxY;
									if (t < ny) {
										ny = t;
									}
								} else if (ny < 0. && pbb.minY >= bb->maxY + y) {
									t = bb->maxY + y - pbb.minY;
									if (t > ny) {
										ny = t;
									}
								}
							}
						}
					}
				}
			}
		}
		gs.player->y += ny;
		pbb.minY += ny;
		pbb.maxY += ny;
		double nx = gs.player->motX;
		for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
			for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
				for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
					block b = getBlockWorld(gs.world, x, y, z);
					if (b > 0 && doesBlockCollide(b)) {
						struct boundingbox* bb = getBlockCollision(b);
						if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
							if (pbb.maxY > bb->minY + y && pbb.minY < bb->maxY + y && pbb.maxZ > bb->minZ + z && pbb.minZ < bb->maxZ + z) {
								double t;
								if (nx > 0. && pbb.maxX <= bb->minX + x) {
									t = bb->minX + x - pbb.maxX;
									if (t < nx) {
										nx = t;
									}
								} else if (nx < 0. && pbb.minX >= bb->maxX + x) {
									t = bb->maxX + x - pbb.minX;
									if (t > nx) {
										nx = t;
									}
								}
							}
						}
					}
				}
			}
		}
		gs.player->x += nx;
		pbb.minX += nx;
		pbb.maxX += nx;
		double nz = gs.player->motZ;
		for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
			for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
				for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
					block b = getBlockWorld(gs.world, x, y, z);
					if (b > 0 && doesBlockCollide(b)) {
						struct boundingbox* bb = getBlockCollision(b);
						if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
							if (pbb.maxX > bb->minX + x && pbb.minX < bb->maxX + x && pbb.maxY > bb->minY + y && pbb.minY < bb->maxY + y) {
								double t;
								if (nz > 0. && pbb.maxZ <= bb->minZ + z) {
									t = bb->minZ + z - pbb.maxZ;
									if (t < nz) {
										nz = t;
									}
								} else if (nz < 0. && pbb.minZ >= bb->maxZ + z) {
									t = bb->maxZ + z - pbb.minZ;
									if (t > nz) {
										nz = t;
									}
								}
							}
						}
					}
				}
			}
		}
		//TODO step
		gs.player->z += nz;
		pbb.minZ += nz;
		pbb.maxZ += nz;
		gs.isCollidedHorizontally = gs.player->motX != nx || gs.player->motZ != nz;
		gs.isCollidedVertically = gs.player->motY != ny;
		gs.player->onGround = gs.isCollidedVertically && gs.player->motY < 0.;
		int32_t bx = floor(gs.player->x);
		int32_t by = floor(gs.player->y - .20000000298023224);
		int32_t bz = floor(gs.player->z);
		block lb = getBlockWorld(gs.world, bx, by, bz);
		if (lb == BLK_AIR) {
			block lbb = getBlockWorld(gs.world, bx, by - 1, bz);
			uint16_t lbi = lbb >> 4;
			if (lbi == BLK_FENCEOAK >> 4 || lbi == BLK_FENCEGATEOAK >> 4 || lbi == BLK_NETHERBRICKFENCE >> 4 || lbi == BLK_FENCEGATESPRUCE >> 4 || lbi == BLK_FENCEGATEBIRCH >> 4 || lbi == BLK_FENCEGATEJUNGLE >> 4 || lbi == BLK_FENCEGATEDARKOAK >> 4 || lbi == BLK_FENCEGATEACACIA >> 4 || lbi == BLK_FENCESPRUCE >> 4 || lbi == BLK_FENCEBIRCH >> 4 || lbi == BLK_FENCEJUNGLE >> 4 || lbi == BLK_FENCEDARKOAK >> 4 || lbi == BLK_FENCEACACIA >> 4 || lbi == BLK_COBBLESTONEWALL >> 4 || lbi == BLK_MOSSYCOBBLESTONEWALL >> 4) {
				lb = lbb;
				by--;
			}
		}
		if (gs.player->motX != nx) gs.player->motX = 0.;
		if (gs.player->motZ != nz) gs.player->motZ = 0.;

		if (gs.player->motY != ny) {
			if (lb != BLK_SLIMEBLOCK || gs.crouching) {
				gs.player->motY = 0.;
			} else {
				gs.player->motY = -gs.player->motY;
			}
		}
		//
		//TODO if on ladder
		if (gs.player->onGround) {
			//TODO: soul sand
		}
		gs.player->motY -= .08;
		gs.player->motY *= .9800000190734863;
		gs.player->motX *= v3;
		gs.player->motZ *= v3;
	}
	struct packet pkt;
	pkt.id = PKT_PLAY_CLIENT_PLAYERPOSLOOK;
	pkt.data.play_client.playerposlook.x = gs.player->x;
	pkt.data.play_client.playerposlook.y = gs.player->y;
	pkt.data.play_client.playerposlook.z = gs.player->z;
	pkt.data.play_client.playerposlook.yaw = gs.player->yaw;
	pkt.data.play_client.playerposlook.pitch = gs.player->pitch;
	pkt.data.play_client.playerposlook.onGround = gs.player->onGround;
	if (writePacket(gs.conn, &pkt) != 0) {
		printf("Failed to write packet: %s\n", strerror(errno));
	}
}

//struct vao tb;
//int tbx = 0;

void drawIngame(float partialTick) {
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	float fov = (gs.sprinting && gs.moveForward && !gs.moveBackward) ? 90 : 70.;
	double whratio = (double) width / (double) height;
	gluPerspective(fov, whratio, 0.05, viewDistance);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	float ppitch = gs.player->pitch * (1. - partialTick) + gs.player->lpitch * partialTick;
	float pyaw = gs.player->yaw * (1. - partialTick) + gs.player->lyaw * partialTick;
	double px = gs.player->x * (1. - partialTick) + gs.player->lx * partialTick;
	double py = gs.player->y * (1. - partialTick) + gs.player->ly * partialTick;
	double pz = gs.player->z * (1. - partialTick) + gs.player->lz * partialTick;
	eyeX = px;
	eyeY = py + 1.62;
	eyeZ = pz;
	double v3 = cos(-gs.player->lyaw * 0.017453292 - PI);
	double v4 = sin(-pyaw * 0.017453292 - PI);
	double v5 = -cos(-ppitch * 0.017453292);
	lookY = sin(-ppitch * 0.017453292);
	lookX = v4 * v5;
	lookZ = v3 * v5;
	double llen = sqrt(lookX * lookX + lookY * lookY + lookZ * lookZ);
	lookY /= llen;
	lookX /= llen;
	lookZ /= llen;
	hnear = 2 * tan(fov / 2.) * 0.05;
	wnear = hnear * whratio;
	hfar = 2 * tan(fov / 2.) * viewDistance;
	wfar = hfar * whratio;
	glRotatef(ppitch, 1., 0., 0.);
	glRotatef(pyaw + 180, 0., 1., 0.);
	if (gs.world->dimension == 1) {
		//TODO
	} else {
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		float ca = 0.;
		{
			int tod = gs.world->time % 24000;
			ca = ((float) tod + partialTick) / 24000. - 0.25;
			if (ca < 0.) {
				ca++;
			} else if (ca > 1.) {
				ca--;
			}
			float cat = ca;
			ca = 1. - (float) ((cos(ca * PI) + 1) / 2.);
			ca = cat + (ca - cat) / 3.;
		}
		float cmod = cos(ca * PI * 2.) * 2. + .5;
		if (cmod < 0.) cmod = 0.;
		if (cmod > 1.) cmod = 1.;
		int pax = floor(gs.player->x);
		//int py = floor(gs.player->y);
		int paz = floor(gs.player->z);
		int bio = getBiome(gs.world, pax, paz);
		float temp = 0.8;
		if (bio == 2) temp = 2.;
		else if (bio == 3) temp = .2;
		else if (bio == 4) temp = .7;
		else if (bio == 5) temp = .25;
		else if (bio == 6) temp = .8;
		else if (bio == 8) temp = 2.;
		else if (bio == 10) temp = 0.;
		else if (bio == 11) temp = 0.;
		else if (bio == 12) temp = 0.;
		else if (bio == 13) temp = 0.;
		else if (bio == 14) temp = .9;
		else if (bio == 15) temp = .9;
		else if (bio == 16) temp = .8;
		else if (bio == 17) temp = 2.;
		else if (bio == 18) temp = .7;
		else if (bio == 19) temp = .25;
		else if (bio == 20) temp = .2;
		else if (bio == 21) temp = .95;
		else if (bio == 22) temp = .95;
		else if (bio == 23) temp = .95;
		else if (bio == 25) temp = .2;
		else if (bio == 26) temp = .05;
		else if (bio == 27) temp = .7;
		else if (bio == 28) temp = .7;
		else if (bio == 29) temp = .7;
		else if (bio == 30) temp = -.5;
		else if (bio == 31) temp = -.5;
		else if (bio == 32) temp = .3;
		else if (bio == 33) temp = .3;
		else if (bio == 34) temp = .2;
		else if (bio == 35) temp = 1.2;
		else if (bio == 36) temp = 1.;
		else if (bio == 37) temp = 2.;
		else if (bio == 38) temp = 2.;
		else if (bio == 39) temp = 2.;
		else if (bio == 129) temp = .8;
		else if (bio == 132) temp = .7;
		else if (bio == 140) temp = 0.;
		else if (bio == 165) temp = 2.;
		else if (bio == 166) temp = 2.;
		else if (bio == 167) temp = 2.;
		else if (bio == 160) temp = .25;
		else if (bio == 131) temp = .2;
		else if (bio == 162) temp = .2;
		else if (bio == 161) temp = .25;
		float fr = .7529412;
		float fg = .84705883;
		float fb = 1.;
		fr *= cmod * .94 + .06;
		fg *= cmod * .94 + .06;
		fb *= cmod * .91 + .09;
		//TODO: sunrise

		temp /= 3.;
		if (temp < -1.) temp = -1.;
		if (temp > 1.) temp = 1.;
		float h = .62222224 - temp * .05;
		float s = .5 + temp * .1;
		float br = 1.;
		float r = 0.;
		float g = 0.;
		float b = 0.;
		h = (h - (float) floor(h)) * 6.;
		float f = h - (float) floor(h);
		float p = br * (1. - s);
		float q = br * (1. - s * f);
		float t = br * (1. - (s * (1. - f)));
		int hi = (int) h;
		if (hi == 0) {
			r = br * 255. + .5;
			g = t * 255. + .5;
			b = p * 255. + .5;
		} else if (hi == 1) {
			r = q * 255. + .5;
			g = br * 255. + .5;
			b = p * 255. + .5;
		} else if (hi == 2) {
			r = p * 255. + .5;
			g = br * 255. + .5;
			b = t * 255. + .5;
		} else if (hi == 3) {
			r = p * 255. + .5;
			g = q * 255. + .5;
			b = br * 255. + .5;
		} else if (hi == 4) {
			r = t * 255. + .5;
			g = p * 255. + .5;
			b = br * 255. + .5;
		} else if (hi == 5) {
			r = br * 255. + .5;
			g = p * 255. + .5;
			b = q * 255. + .5;
		}
		r /= 255.;
		g /= 255.;
		b /= 255.;
		r *= cmod;
		g *= cmod;
		b *= cmod;
		//TODO: some kind of render distance effect on fog color
		glClearColor(fr, fg, fb, 1.);
		//TODO: rain
		//TODO: thunder
		glColor4f(r, g, b, 1.);
		glDepthMask (GL_FALSE);
		drawQuads(&skybox);
		glDepthMask (GL_TRUE);
		glColor4f(1., 1., 1., 1.);
		glEnable(GL_TEXTURE_2D);
	}
	glTranslatef(-px, -(py + 1.62), -pz);
	drawWorld(gs.world);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0., swidth, sheight, 0., -1., 1.);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear (GL_DEPTH_BUFFER_BIT);
	if (gs.inMenu) {
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_DEPTH_TEST);
		glDepthMask (GL_FALSE);
		glColor4f(0.0625, 0.0625, 0.0625, 0.75);
		glBegin (GL_QUADS);
		glVertex3f(0., sheight, 0.);
		glVertex3f(swidth, sheight, 0.);
		glVertex3f(swidth, 0., 0.);
		glVertex3f(0., 0., 0.);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		drawString("Game menu", swidth / 2 - stringWidth("Game menu") / 2, 36, 16777215);
		glColor4f(1., 1., 1., 1.);
		glEnable(GL_DEPTH_TEST);
		glDepthMask (GL_TRUE);
		if (drawButton(swidth / 2 - 100, sheight / 4 + 120 - 16, 200, 20, "Disconnect", 1) && mouseButton == 0) {

		}
		if (drawButton(swidth / 2 - 100, sheight / 4 + 24 - 16, 200, 20, "Back to Game", 0) && mouseButton == 0) {
			gs.inMenu = 0;
		}
		if (drawButton(swidth / 2 - 100, sheight / 4 + 96 - 16, 98, 20, "Options", 1) && mouseButton == 0) {

		}
		drawButton(swidth / 2 + 2, sheight / 4 + 96 - 16, 98, 20, "Open to LAN", 1);
		if (drawButton(swidth / 2 - 100, sheight / 4 + 48 - 16, 98, 20, "Achievements", 1) && mouseButton == 0) {

		}
		if (drawButton(swidth / 2 + 2, sheight / 4 + 48 - 16, 98, 20, "Statistics", 1) && mouseButton == 0) {

		}
	}
	glColor4f(1., 1., 1., 1.);
}

void runNetwork(struct conn* conn) {
	spawnedIn = 0;
	gs.conn = conn;
	viewDistance = 16. * 25.;
	struct packet pkt;
	struct packet rpkt;
	while (1) {
		if (readPacket(conn, &pkt)) {
			printf("closed\n");
			return;
		}
		//printf("recv: %i\n", pkt.id);
		if (pkt.id == PKT_PLAY_SERVER_SPAWNOBJECT) {

		} else if (pkt.id == PKT_PLAY_SERVER_SPAWNEXPERIENCEORB) {

		} else if (pkt.id == PKT_PLAY_SERVER_SPAWNGLOBALENTITY) {

		} else if (pkt.id == PKT_PLAY_SERVER_SPAWNMOB) {

		} else if (pkt.id == PKT_PLAY_SERVER_SPAWNPAINTING) {

		} else if (pkt.id == PKT_PLAY_SERVER_SPAWNPLAYER) {

		} else if (pkt.id == PKT_PLAY_SERVER_ANIMATION) {

		} else if (pkt.id == PKT_PLAY_SERVER_STATISTICS) {

		} else if (pkt.id == PKT_PLAY_SERVER_BLOCKBREAKANIMATION) {

		} else if (pkt.id == PKT_PLAY_SERVER_UPDATEBLOCKENTITY) {

		} else if (pkt.id == PKT_PLAY_SERVER_BLOCKACTION) {

		} else if (pkt.id == PKT_PLAY_SERVER_BLOCKCHANGE) {
			setBlockWorld(gs.world, pkt.data.play_server.blockchange.blockID, pkt.data.play_server.blockchange.pos.x, pkt.data.play_server.blockchange.pos.y, pkt.data.play_server.blockchange.pos.z);
		} else if (pkt.id == PKT_PLAY_SERVER_BOSSBAR) {

		} else if (pkt.id == PKT_PLAY_SERVER_SERVERDIFFICULTY) {
			gs.difficulty = pkt.data.play_server.serverdifficulty.difficulty;
		} else if (pkt.id == PKT_PLAY_SERVER_TABCOMPLETE) {

		} else if (pkt.id == PKT_PLAY_SERVER_CHATMESSAGE) {

		} else if (pkt.id == PKT_PLAY_SERVER_MULTIBLOCKCHANGE) {
			struct chunk* ch = getChunk(gs.world, pkt.data.play_server.multiblockchange.x, pkt.data.play_server.multiblockchange.z);
			if (ch == NULL) {
				free(pkt.data.play_server.multiblockchange.records);
				continue;
			}
			for (size_t i = 0; i < pkt.data.play_server.multiblockchange.record_count; i++) {
				struct mbc_record* mbcr = &pkt.data.play_server.multiblockchange.records[i];
				setBlockChunk(ch, mbcr->blockID, mbcr->x, mbcr->y, mbcr->z);
			}
			free(pkt.data.play_server.multiblockchange.records);
		} else if (pkt.id == PKT_PLAY_SERVER_CONFIRMTRANSACTION) {

		} else if (pkt.id == PKT_PLAY_SERVER_CLOSEWINDOW) {

		} else if (pkt.id == PKT_PLAY_SERVER_OPENWINDOW) {

		} else if (pkt.id == PKT_PLAY_SERVER_WINDOWITEMS) {

		} else if (pkt.id == PKT_PLAY_SERVER_WINDOWPROPERTY) {

		} else if (pkt.id == PKT_PLAY_SERVER_SETSLOT) {

		} else if (pkt.id == PKT_PLAY_SERVER_SETCOOLDOWN) {

		} else if (pkt.id == PKT_PLAY_SERVER_PLUGINMESSAGE) {

		} else if (pkt.id == PKT_PLAY_SERVER_NAMEDSOUNDEFFECT) {

		} else if (pkt.id == PKT_PLAY_SERVER_DISCONNECT) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYSTATUS) {

		} else if (pkt.id == PKT_PLAY_SERVER_EXPLOSION) {
			for (size_t i = 0; i < pkt.data.play_server.explosion.record_count; i++) {
				struct exp_record* expr = &pkt.data.play_server.explosion.records[i];
				setBlockWorld(gs.world, 0, expr->x + (int32_t) pkt.data.play_server.explosion.x, expr->y + (int32_t) pkt.data.play_server.explosion.y, expr->z + (int32_t) pkt.data.play_server.explosion.z);
			}
			free(pkt.data.play_server.explosion.records);
			gs.player->motX += pkt.data.play_server.explosion.velX;
			gs.player->motY += pkt.data.play_server.explosion.velY;
			gs.player->motZ += pkt.data.play_server.explosion.velZ;
		} else if (pkt.id == PKT_PLAY_SERVER_UNLOADCHUNK) {
			struct chunk* ch = getChunk(gs.world, pkt.data.play_server.unloadchunk.chunkX, pkt.data.play_server.unloadchunk.chunkZ);
			if (ch != NULL) {
				removeChunk(gs.world, ch);
				freeChunk(ch);
			}
		} else if (pkt.id == PKT_PLAY_SERVER_CHANGEGAMESTATE) {

		} else if (pkt.id == PKT_PLAY_SERVER_KEEPALIVE) {
			rpkt.id = PKT_PLAY_CLIENT_KEEPALIVE;
			rpkt.data.play_client.keepalive.key = pkt.data.play_server.keepalive.key;
			writePacket(conn, &rpkt);
		} else if (pkt.id == PKT_PLAY_SERVER_CHUNKDATA) {
			unsigned char* data = pkt.data.play_server.chunkdata.data;
			size_t size = pkt.data.play_server.chunkdata.size;
			struct chunk* chunk = pkt.data.play_server.chunkdata.continuous ? newChunk(pkt.data.play_server.chunkdata.x, pkt.data.play_server.chunkdata.z) : getChunk(gs.world, pkt.data.play_server.chunkdata.x, pkt.data.play_server.chunkdata.z);
			size_t bo = 0;
			for (int32_t x = 0; x < 16; x++) { // could be more!
				if (!(pkt.data.play_server.chunkdata.bitMask & (1 << x))) {
					continue;
				}
				if (size < 1) {
					if (pkt.data.play_server.chunkdata.continuous) free(chunk);
					goto rcmp;
				}
				unsigned char bpb = data[0];
				unsigned char bpbr = bpb;
				data++;
				size--;
				int32_t plen = 0;
				int32_t* pal = NULL;
				if (bpb == 0) {
					bpbr = 13;
				} else {
					int rx = readVarInt(&plen, data, size);
					data += rx;
					size -= rx;
					pal = malloc(sizeof(int32_t) * plen);
					for (int32_t i = 0; i < plen; i++) {
						rx = readVarInt(&pal[i], data, size);
						data += rx;
						size -= rx;
					}
				}
				int32_t bks_l;
				int rx = readVarInt(&bks_l, data, size);
				data += rx;
				size -= rx;
				bks_l *= 8;
				if (bks_l < 0 || bks_l > size || bpbr > 16) {
					if (pal != NULL) free(pal);
					if (pkt.data.play_server.chunkdata.continuous) free(chunk);
					goto rcmp;
				}
				for (size_t i = 0; i < bks_l; i += 8) {
					swapEndian(data + i, 8);
				}
				size_t bs = 4096 * bpbr;
				size_t tx = (bs + (bs % 8) + bo);
				if (tx / 8 + (tx % 8 > 0 ? 1 : 0) > bks_l) {
					if (pal != NULL) free(pal);
					if (pkt.data.play_server.chunkdata.continuous) free(chunk);
					goto rcmp;
				}
				block cv = 0;
				unsigned char cvi = 0;
				int16_t bi = 0;
				for (size_t i = bo; i < bs + bo; i++) {
					unsigned char bit = data[i / 8] & (1 << (i % 8));
					if (bit) cv |= (1 << cvi);
					cvi++;
					if (cvi == bpbr) {
						cvi = 0;
						if (bpb == 0) {
							chunk->blocks[bi & 0x0f][(bi & 0xf0) >> 4][(x * 16) + ((bi & 0xf00) >> 8)] = cv;
						} else if (plen > 0 && cv < plen) {
							chunk->blocks[bi & 0x0f][(bi & 0xf0) >> 4][(x * 16) + ((bi & 0xf00) >> 8)] = pal[cv];
						} else {
							chunk->blocks[bi & 0x0f][(bi & 0xf0) >> 4][(x * 16) + ((bi & 0xf00) >> 8)] = 0;
						}
						bi++;
						cv = 0;
					}
				}
				bo = bs % 8;
				size -= bs / 8;
				data += bs / 8;
				for (size_t i = 0; i < 4096; i++) {
					unsigned char lb = data[0];
					if (i % 2 == 0) {
						lb >>= 4;
					} else {
						lb &= 0x0f;
					}
					if (i % 2 == 0) {
						chunk->blockLight[i & 0x0f][(i & 0xf0) >> 4][(x * 8) + ((i & 0xf00) >> 8)] = lb << 4;
					} else {
						chunk->blockLight[i & 0x0f][(i & 0xf0) >> 4][(x * 8) + ((i & 0xf00) >> 9)] |= lb & 0x0f;
					}
					if (i % 2 == 1) {
						data++;
						size--;
					}
				}
				if (gs.world->dimension == 0) {
					chunk->skyLight = malloc(2048);
					for (size_t i = 0; i < 4096; i++) {
						unsigned char lb = data[0];
						if (i % 2 == 0) {
							lb >>= 4;
						} else {
							lb &= 0x0f;
						}
						if (i % 2 == 0) {
							chunk->skyLight[i / 2] = lb << 4;
						} else {
							chunk->skyLight[i / 2] |= lb & 0x0f;
						}
						if (i % 2 == 1) {
							data++;
							size--;
						}
					}
				}
			}
			if (pkt.data.play_server.chunkdata.continuous) {
				struct chunk* cc = getChunk(gs.world, pkt.data.play_server.chunkdata.x, pkt.data.play_server.chunkdata.z);
				if (cc != NULL) {
					free(cc); //remove chunk does not dereference the chunk
					removeChunk(gs.world, cc);
				}
				addChunk(gs.world, chunk);
			}
			free(pkt.data.play_server.chunkdata.data);
		} else if (pkt.id == PKT_PLAY_SERVER_EFFECT) {

		} else if (pkt.id == PKT_PLAY_SERVER_PARTICLE) {

		} else if (pkt.id == PKT_PLAY_SERVER_JOINGAME) {
			gs.difficulty = pkt.data.play_server.joingame.difficulty;
			gs.gamemode = pkt.data.play_server.joingame.gamemode;
			gs.maxPlayers = pkt.data.play_server.joingame.maxPlayers;
			gs.player = newEntity(pkt.data.play_server.joingame.eid, 0., 0., 0., ENTITY_OURPLAYER, 0., 0.);
			gs.reducedDebugInfo = pkt.data.play_server.joingame.reducedDebugInfo;
			gs.world = newWorld();
			spawnEntity(gs.world, gs.player);
			gs.world->levelType = pkt.data.play_server.joingame.levelType;
			gs.world->dimension = pkt.data.play_server.joingame.dimension;
		} else if (pkt.id == PKT_PLAY_SERVER_MAP) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYRELMOVE) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYLOOKRELMOVE) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYLOOK) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITY) {

		} else if (pkt.id == PKT_PLAY_SERVER_VEHICLEMOVE) {

		} else if (pkt.id == PKT_PLAY_SERVER_OPENSIGNEDITOR) {

		} else if (pkt.id == PKT_PLAY_SERVER_PLAYERABILITIES) {
			//todo
			if (!spawnedIn && 0) {
				rpkt.id = PKT_PLAY_CLIENT_PLUGINMESSAGE;
				rpkt.data.play_client.pluginmessage.channel = "MC|Brand";
				rpkt.data.play_client.pluginmessage.data = "vanilla/remake in C";
				rpkt.data.play_client.pluginmessage.data_size = 19;
				writePacket(conn, &rpkt);
				rpkt.id = PKT_PLAY_CLIENT_CLIENTSTATUS;
				rpkt.data.play_client.clientstatus.actionID = 0;
				writePacket(conn, &rpkt);
			}
		} else if (pkt.id == PKT_PLAY_SERVER_COMBATEVENT) {

		} else if (pkt.id == PKT_PLAY_SERVER_PLAYERLISTITEM) {

		} else if (pkt.id == PKT_PLAY_SERVER_PLAYERPOSLOOK) {
			gs.player->x = ((pkt.data.play_server.playerposlook.flags & 0x01) == 0x01 ? gs.player->x : 0.) + pkt.data.play_server.playerposlook.x;
			gs.player->y = ((pkt.data.play_server.playerposlook.flags & 0x02) == 0x02 ? gs.player->y : 0.) + pkt.data.play_server.playerposlook.y;
			gs.player->z = ((pkt.data.play_server.playerposlook.flags & 0x04) == 0x04 ? gs.player->z : 0.) + pkt.data.play_server.playerposlook.z;
			gs.player->lx = gs.player->x;
			gs.player->ly = gs.player->y;
			gs.player->lz = gs.player->z;
			gs.player->pitch = ((pkt.data.play_server.playerposlook.flags & 0x08) == 0x08 ? gs.player->pitch : 0.) + pkt.data.play_server.playerposlook.y;
			gs.player->yaw = ((pkt.data.play_server.playerposlook.flags & 0x10) == 0x10 ? gs.player->yaw : 0.) + pkt.data.play_server.playerposlook.z;
			gs.player->health = 20.;
			printf("spawned in at: %f, %f, %f\n", gs.player->x, gs.player->y, gs.player->z);
			rpkt.id = PKT_PLAY_CLIENT_TELEPORTCONFIRM;
			rpkt.data.play_client.teleportconfirm.teleportID = pkt.data.play_server.playerposlook.teleportID;
			writePacket(conn, &rpkt);
			running = 1;
			spawnedIn = 1;
		} else if (pkt.id == PKT_PLAY_SERVER_USEBED) {

		} else if (pkt.id == PKT_PLAY_SERVER_DESTROYENTITIES) {

		} else if (pkt.id == PKT_PLAY_SERVER_REMOVEENTITYEFFECT) {

		} else if (pkt.id == PKT_PLAY_SERVER_RESOURCEPACKSEND) {

		} else if (pkt.id == PKT_PLAY_SERVER_RESPAWN) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYHEADLOOK) {

		} else if (pkt.id == PKT_PLAY_SERVER_WORLDBORDER) {

		} else if (pkt.id == PKT_PLAY_SERVER_CAMERA) {

		} else if (pkt.id == PKT_PLAY_SERVER_HELDITEMCHANGE) {

		} else if (pkt.id == PKT_PLAY_SERVER_DISPLAYSCOREBOARD) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYMETADATA) {

		} else if (pkt.id == PKT_PLAY_SERVER_ATTACHENTITY) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYVELOCITY) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYEQUIPMENT) {

		} else if (pkt.id == PKT_PLAY_SERVER_SETEXPERIENCE) {

		} else if (pkt.id == PKT_PLAY_SERVER_UPDATEHEALTH) {
			gs.player->health = pkt.data.play_server.updatehealth.health;
		} else if (pkt.id == PKT_PLAY_SERVER_SCOREBOARDOBJECTIVE) {

		} else if (pkt.id == PKT_PLAY_SERVER_SETPASSENGERS) {

		} else if (pkt.id == PKT_PLAY_SERVER_TEAMS) {

		} else if (pkt.id == PKT_PLAY_SERVER_UPDATESCORE) {

		} else if (pkt.id == PKT_PLAY_SERVER_SPAWNPOSITION) {
			memcpy(&gs.world->spawnpos, &pkt.data.play_server.spawnposition.pos, sizeof(struct encpos));
		} else if (pkt.id == PKT_PLAY_SERVER_TIMEUPDATE) {
			gs.world->time = pkt.data.play_server.timeupdate.time;
			gs.world->age = pkt.data.play_server.timeupdate.worldAge;
		} else if (pkt.id == PKT_PLAY_SERVER_TITLE) {

		} else if (pkt.id == PKT_PLAY_SERVER_UPDATESIGN) {

		} else if (pkt.id == PKT_PLAY_SERVER_SOUNDEFFECT) {

		} else if (pkt.id == PKT_PLAY_SERVER_PLAYERLISTHEADERFOOTER) {

		} else if (pkt.id == PKT_PLAY_SERVER_COLLECTITEM) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYTELEPORT) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYPROPERTIES) {

		} else if (pkt.id == PKT_PLAY_SERVER_ENTITYEFFECT) {

		}
		rcmp: ;
	}
}
