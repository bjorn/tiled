/*-
 * #%L
 * This file is part of libtiled-java.
 * %%
 * Copyright (C) 2004 - 2020 Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright (C) 2016 - 2020 Mike Thomas <mikepthomas@outlook.com>
 * Copyright (C) 2020 Adam Hornacek <adam.hornacek@icloud.com>
 * %%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * #L%
 */
package org.mapeditor.io;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;

import static org.junit.Assert.*;

import org.junit.Test;

import org.mapeditor.core.Map;
import org.mapeditor.core.ObjectGroup;
import org.mapeditor.core.Orientation;
import org.mapeditor.core.StaggerAxis;
import org.mapeditor.core.StaggerIndex;
import org.mapeditor.core.TileLayer;
import org.mapeditor.core.TileSet;

import javax.xml.bind.JAXBException;

public class MapReaderTest {

    @Test
    public void testAcceptValidFilenames() throws JAXBException {
        // Arrange
        TMXMapReader reader = new TMXMapReader();

        // Assert
        assertTrue(reader.accept(new File("example.tmx")));
        assertTrue(reader.accept(new File("/tmp/example.tmx")));
        assertTrue(reader.accept(new File("/tmp/example.tsx")));
        assertTrue(reader.accept(new File("/tmp/example.tmx.gz")));
        assertFalse(reader.accept(new File("/tmp/example.txt")));
        assertFalse(reader.accept(new File("/tmp/example.xml")));
        assertFalse(reader.accept(new File("/tmp/example.exe")));
        assertFalse(reader.accept(new File("/tmp/example")));
    }

    @Test
    public void testReadingSewersMap() throws Exception {
        // Arrange
        URL url = getUrlFromResources("sewers/sewers.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkSewersMap(map);
    }

    @Test
    public void testReadingSewersMapJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("sewers/sewers.tmx"));
        checkSewersMap(map);
    }

    @Test
    public void testMultiTileset() throws Exception {
        final Map map = new TMXMapReader().readMap(getJarURL("multi-tileset/desert.tmx"));
        final List<TileSet> tilesets = map.getTileSets();
        assertEquals(2, tilesets.size());
        assertEquals(Integer.valueOf(1), tilesets.get(0).getFirstgid());
        assertEquals(Integer.valueOf(49), tilesets.get(1).getFirstgid());
    }

    private void checkSewersMap(final Map map) {
        assertEquals(Orientation.ORTHOGONAL, map.getOrientation());
        assertEquals(50, map.getWidth());
        assertEquals(50, map.getHeight());
        assertEquals(24, map.getTileWidth());
        assertEquals(24, map.getTileHeight());
        assertEquals(3, map.getLayerCount());

        TileLayer bottom = (TileLayer) map.getLayer(0);
        assertEquals("Bottom", bottom.getName());
        assertEquals(50, bottom.getWidth());
        assertEquals(50, bottom.getHeight());
        assertNotNull(bottom.getTileAt(0, 0));

        TileLayer top = (TileLayer) map.getLayer(1);
        assertEquals("Top", top.getName());
        assertEquals(50, top.getWidth());
        assertEquals(50, top.getHeight());
        assertEquals(0.49f, top.getOpacity(), 0.01);

        ObjectGroup objectGroup = (ObjectGroup) map.getLayer(2);
        assertEquals("Objects", objectGroup.getName());
    }

    @Test
    public void testReadingCsvMap() throws Exception {
        // Arrange
        URL url = getUrlFromResources("csvmap/csvmap.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkCsvMap(map);
    }

    @Test
    public void testReadingCsvMapJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("csvmap/csvmap.tmx"));
        checkCsvMap(map);
    }

    private void checkCsvMap(final Map map) {
        assertEquals(Orientation.ORTHOGONAL, map.getOrientation());
        assertEquals(100, map.getWidth());
        assertEquals(100, map.getHeight());
        assertEquals(32, map.getTileWidth());
        assertEquals(32, map.getTileHeight());
        assertEquals(1, map.getLayerCount());

        TileLayer layer = (TileLayer) map.getLayer(0);
        assertNotNull(layer.getTileAt(0, 0));
    }

    @Test
    public void testReadingCsvMapEmbeddedImageCollection() throws Exception {
        // Arrange
        URL url = getUrlFromResources("csvmap_embedded_image_collection/csvmap_embedded_image_collection.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkCsvMapEmbeddedImageCollection(map);
    }

    @Test
    public void testReadingCsvMapEmbeddedImageCollectionJar() throws Exception {
        Map map = new TMXMapReader().readMap(
                getJarURL("csvmap_embedded_image_collection/csvmap_embedded_image_collection.tmx"));
        checkCsvMapEmbeddedImageCollection(map);
    }

    private void checkCsvMapEmbeddedImageCollection(final Map map) {
        assertEquals(Orientation.ORTHOGONAL, map.getOrientation());
        assertEquals(3, map.getWidth());
        assertEquals(1, map.getHeight());
        assertEquals(32, map.getTileWidth());
        assertEquals(32, map.getTileHeight());
        assertEquals(1, map.getLayerCount());

        TileLayer layer = (TileLayer) map.getLayer(0);
        assertNotNull(layer.getTileAt(0, 0));
        assertNotNull(layer.getTileAt(2, 0));

        TileSet tileset = layer.getMap().getTileSets().get(0);
        assertEquals(3, tileset.getMaxTileId());
    }

    @Test
    public void testReadingDesertMap() throws Exception {
        // Arrange
        URL url = getUrlFromResources("desert/desert.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkDesertMap(map);
    }

    @Test
    public void testReadingDesertMapJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("desert/desert.tmx"));
        checkDesertMap(map);
    }

    private void checkDesertMap(final Map map) {
        assertEquals(Orientation.ORTHOGONAL, map.getOrientation());
        assertEquals(40, map.getWidth());
        assertEquals(40, map.getHeight());
        assertEquals(32, map.getTileWidth());
        assertEquals(32, map.getTileHeight());
        assertEquals(1, map.getLayerCount());

        TileLayer layer = (TileLayer) map.getLayer(0);
        assertNotNull(layer.getTileAt(0, 0));
    }

    @Test(expected = IOException.class)
    public void testErrorReadingImage() throws Exception {
        URL url = getUrlFromResources("desert_missing_image/desert.tmx");
        new TMXMapReader().readMap(url.getPath());
    }

    @Test(expected = IOException.class)
    public void testErrorReadingImageJar() throws Exception {
        new TMXMapReader().readMap(getJarURL("desert_missing_image/desert.tmx"));
    }

    @Test(expected = IOException.class)
    public void testErrorReadingTileset() throws Exception {
        URL url = getUrlFromResources("desert_missing_tileset/desert.tmx");
        new TMXMapReader().readMap(url.getPath());
    }

    @Test(expected = IOException.class)
    public void testErrorReadingTilesetJar() throws Exception {
        new TMXMapReader().readMap(getJarURL("desert_missing_tileset/desert.tmx"));
    }

    @Test
    public void testReadingExampleOutsideMap() throws Exception {
        // Arrange
        URL url = getUrlFromResources("orthogonal-outside/orthogonal-outside.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkOutsideMap(map);
    }

    @Test
    public void testReadingExampleOutsideMapJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("orthogonal-outside/orthogonal-outside.tmx"));
        checkOutsideMap(map);
    }

    private void checkOutsideMap(final Map map) {
        assertEquals(Orientation.ORTHOGONAL, map.getOrientation());
        assertEquals(45, map.getWidth());
        assertEquals(31, map.getHeight());
        assertEquals(16, map.getTileWidth());
        assertEquals(16, map.getTileHeight());
        assertEquals(3, map.getLayerCount());

        TileLayer layer = (TileLayer) map.getLayer(0);
        assertNotNull(layer.getTileAt(0, 0));
    }

    @Test
    public void testReadingPerspectiveWallsMap() throws Exception {
        // Arrange
        URL url = getUrlFromResources("perspective_walls/perspective_walls.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkPerspectiveWalls(map);
    }

    @Test
    public void testReadingPerspectiveWallsMapJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("perspective_walls/perspective_walls.tmx"));
        checkPerspectiveWalls(map);
    }

    private void checkPerspectiveWalls(final Map map) {
        assertEquals(Orientation.ORTHOGONAL, map.getOrientation());
        assertEquals(32, map.getWidth());
        assertEquals(32, map.getHeight());
        assertEquals(31, map.getTileWidth());
        assertEquals(31, map.getTileHeight());
        assertEquals(3, map.getLayerCount());

        TileLayer layer = (TileLayer) map.getLayer(0);
        assertNotNull(layer.getTileAt(6, 11));
    }

    @Test
    public void testReadingHexagonalMap() throws Exception {
        // Arrange
        URL url = getUrlFromResources("hexagonal-mini/hexagonal-mini.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkHexagonalMap(map);
    }

    @Test
    public void testReadingHexagonalMapJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("hexagonal-mini/hexagonal-mini.tmx"));
        checkHexagonalMap(map);
    }

    private void checkHexagonalMap(final Map map) {
        assertEquals(Orientation.HEXAGONAL, map.getOrientation());
        assertEquals(20, map.getWidth());
        assertEquals(20, map.getHeight());
        assertEquals(14, map.getTileWidth());
        assertEquals(12, map.getTileHeight());
        assertEquals(6, map.getHexSideLength().intValue());
        assertEquals(StaggerAxis.Y, map.getStaggerAxis());
        assertEquals(StaggerIndex.ODD, map.getStaggerIndex());
        assertEquals(1, map.getLayerCount());
    }

    @Test
    public void testReadingStaggeredMap() throws Exception {
        // Arrange
        URL url = getUrlFromResources("staggered.tmx");

        // Act
        Map map = new TMXMapReader().readMap(url.getPath());
        checkStaggeredMap(map);
    }

    @Test
    public void testReadingStaggeredMapJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("staggered.tmx"));
        checkStaggeredMap(map);
    }

    private void checkStaggeredMap(final Map map) {
        assertEquals(Orientation.STAGGERED, map.getOrientation());
        assertEquals(9, map.getWidth());
        assertEquals(9, map.getHeight());
        assertEquals(32, map.getTileWidth());
        assertEquals(32, map.getTileHeight());
        assertEquals(StaggerAxis.Y, map.getStaggerAxis());
        assertEquals(StaggerIndex.ODD, map.getStaggerIndex());
        assertEquals(1, map.getLayerCount());
    }

    @Test
    public void testReadingFlippedTiles() throws Exception {
        // Arrange
        URL url = getUrlFromResources("flipped/flipped.tmx");

        // Act – test file URL read
        Map map = new TMXMapReader().readMap(url);
        checkFlippedTiles(map);
    }

    @Test
    public void testReadingFlippedTilesJar() throws Exception {
        Map map = new TMXMapReader().readMap(getJarURL("flipped/flipped.tmx"));
        checkFlippedTiles(map);
    }

    private void checkFlippedTiles(final Map map) {
        assertEquals(Orientation.ORTHOGONAL, map.getOrientation());
        assertEquals(4, map.getWidth());
        assertEquals(1, map.getHeight());
        assertEquals(32, map.getTileWidth());
        assertEquals(32, map.getTileHeight());
        assertEquals(1, map.getLayerCount());

        TileLayer layer = (TileLayer) map.getLayer(0);
        assertNotNull(layer.getTileAt(0, 0));

        assertTrue(layer.isFlippedHorizontally(0, 0));
        assertFalse(layer.isFlippedVertically(0, 0));
        assertFalse(layer.isFlippedDiagonally(0, 0));

        assertFalse(layer.isFlippedHorizontally(1, 0));
        assertTrue(layer.isFlippedVertically(1, 0));
        assertFalse(layer.isFlippedDiagonally(1, 0));

        assertTrue(layer.isFlippedHorizontally(2, 0));
        assertTrue(layer.isFlippedVertically(2, 0));
        assertFalse(layer.isFlippedDiagonally(2, 0));

        assertFalse(layer.isFlippedHorizontally(3, 0));
        assertFalse(layer.isFlippedVertically(3, 0));
        assertFalse(layer.isFlippedDiagonally(3, 0));
    }

    private URL getUrlFromResources(String filename) {
        ClassLoader classLoader = this.getClass().getClassLoader();
        return classLoader.getResource(filename);
    }

    private URL getJarURL(final String filename) throws MalformedURLException {
        URL url = getUrlFromResources("resources.jar");
        return new URL("jar:" + url.toString() + "!/" + filename);
    }

    @Test
    public void testReadmapWithSearchDirectory() throws Exception {
        ClassLoader loader = Thread.currentThread().getContextClassLoader();
        String resourceName = "relative_paths/relative_paths.tmx";
        URL url = loader.getResource(resourceName);
        String parentDirectory = new File(url.getFile()).getParent();
        InputStream in = loader.getResourceAsStream(resourceName);

        Map map = new TMXMapReader().readMap(in, parentDirectory);
        assertEquals(1, map.getTileSets().size());
    }
}
